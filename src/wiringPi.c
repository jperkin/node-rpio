/*
 * wiringPi:
 *	Arduino compatable (ish) Wiring library for the Raspberry Pi
 *	Copyright (c) 2012 Gordon Henderson
 *	Additional code for pwmSetClock by Chris Hall <chris@kchall.plus.com>
 *
 *	Thanks to code samples from Gert Jan van Loo and the
 *	BCM2835 ARM Peripherals manual, however it's missing
 *	the clock section /grr/mutter/
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with wiringPi.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

// Revisions:
//  01 Sep 2017
//      Added modification for NanoPi
//	19 Jul 2012:
//		Moved to the LGPL
//		Added an abstraction layer to the main routines to save a tiny
//		bit of run-time and make the clode a little cleaner (if a little
//		larger)
//		Added waitForInterrupt code
//		Added piHiPri code
//
//	 9 Jul 2012:
//		Added in support to use the /sys/class/gpio interface.
//	 2 Jul 2012:
//		Fixed a few more bugs to do with range-checking when in GPIO mode.
//	11 Jun 2012:
//		Fixed some typos.
//		Added c++ support for the .h file
//		Added a new function to allow for using my "pin" numbers, or native
//			GPIO pin numbers.
//		Removed my busy-loop delay and replaced it with a call to delayMicroseconds
//
//	02 May 2012:
//		Added in the 2 UART pins
//		Change maxPins to numPins to more accurately reflect purpose


#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <limits.h>

#include "wiringPi.h"

#ifndef TRUE
#define TRUE (1==1)
#define FALSE (1==2)
#endif

// Environment Variables

#define ENV_DEBUG "WIRINGPI_DEBUG"
#define ENV_CODES "WIRINGPI_CODES"


// Access from ARM Running Linux
//	Taken from Gert/Doms code. Some of this is not in the manual
//	that I can find )-:

#define BLOCK_SIZE  (4*1024)

// Locals to hold pointers to the hardware

static volatile uint32_t *gpio;
static volatile uint32_t *pwm;
static volatile uint32_t *clk;
static volatile uint32_t *pads;

#ifdef USE_TIMER
static volatile uint32_t *timer;
static volatile uint32_t *timerIrqRaw;
#endif

#define SUNXI_GPIO_BASE (0x01C20800)
#define MAP_SIZE (4096*2)
#define MAP_MASK (MAP_SIZE - 1)

#define GPIO_PADS  (0x00100000)
#define CLOCK_BASE (0x00101000)
#define GPIO_BASE  (0x01C20000)
#define GPIO_TIMER (0x0000B000)
#define GPIO_PWM   (0x01C21000)

#define SUNXI_PWM_CTRL_REG    (0x01C21400)
#define SUNXI_PWM_CH0_PERIOD  (0x01C21404)

#define SUNXI_PWM_CH0_EN   (1 << 4)
#define SUNXI_PWM_CH0_ACT_STA  (1 << 5)
#define SUNXI_PWM_SCLK_CH0_GATING (1 << 6)
#define SUNXI_PWM_CH0_MS_MODE  (1 << 7) //pulse mode
#define SUNXI_PWM_CH0_PUL_START  (1 << 8)

#define SUNXI_PWM_CH1_EN   (1 << 19)
#define SUNXI_PWM_CH1_ACT_STA  (1 << 20)
#define SUNXI_PWM_SCLK_CH1_GATING (1 << 21)
#define SUNXI_PWM_CH1_MS_MODE  (1 << 22) //pulse mode
#define SUNXI_PWM_CH1_PUL_START  (1 << 23)


#define PWM_CLK_DIV_120  0
#define PWM_CLK_DIV_180  1
#define PWM_CLK_DIV_240  2
#define PWM_CLK_DIV_360  3
#define PWM_CLK_DIV_480  4
#define PWM_CLK_DIV_12K  8
#define PWM_CLK_DIV_24K  9
#define PWM_CLK_DIV_36K  10
#define PWM_CLK_DIV_48K  11
#define PWM_CLK_DIV_72K  12

static int wiringPinMode = WPI_MODE_UNINITIALISED;
/*end 2014.09.18*/

// Time for easy calculations

static uint64_t epochMilli, epochMicro;

// Misc

static volatile int pinPass = -1;

// Debugging & Return codes

int wiringPiDebug = FALSE; // guenter FALSE ;
int wiringPiReturnCodes = FALSE;

// ISR Data

//static void (*isrFunctions [64])(void);


static int upDnConvert[3] = {7, 7, 5};

static int pwmmode = 0;

/*end 20140918*/

/*
 * Functions
 *********************************************************************************
 */


uint32_t readl(uint32_t addr) {
    uint32_t val = 0;
    uint32_t mmap_base = (addr & ~MAP_MASK);
    uint32_t mmap_seek = ((addr - mmap_base) >> 2);
    val = *(gpio + mmap_seek);
    return val;
    
}

void writel(uint32_t val, uint32_t addr) {
    uint32_t mmap_base = (addr & ~MAP_MASK);
    uint32_t mmap_seek = ((addr - mmap_base) >> 2);
    *(gpio + mmap_seek) = val;
}

const char * int2bin(uint32_t param) {
    int bits = sizeof(uint32_t)*CHAR_BIT;
    static char buffer[sizeof(uint32_t)*CHAR_BIT + 1];
    char chars[2] = {'0', '1'};
    int i,j,offset;
    for (i = 0; i < bits; i++) {
        j = bits - i - 1;
        offset = (param & (1 << j)) >> j;
        buffer[i] = chars[offset];
    }
    buffer[bits] = '\0';
    return buffer;
}


void print_pwm_reg() {
    uint32_t val = readl(SUNXI_PWM_CTRL_REG);
    uint32_t val2 = readl(SUNXI_PWM_CH0_PERIOD);
    if (wiringPiDebug) {
        printf("SUNXI_PWM_CTRL_REG: %s\n", int2bin(val));
        printf("SUNXI_PWM_CH0_PERIOD: %s\n", int2bin(val2));
    }
}

void sunxi_pwm_set_enable(int en) {
    int val = 0;
    val = readl(SUNXI_PWM_CTRL_REG);
    if (en) {
        val |= (SUNXI_PWM_CH0_EN | SUNXI_PWM_SCLK_CH0_GATING);
    }
    else {
        val &= ~(SUNXI_PWM_CH0_EN | SUNXI_PWM_SCLK_CH0_GATING);
    }
    if (wiringPiDebug)
        printf(">>function%s,no:%d,enable? :0x%x\n", __func__, __LINE__, val);
    writel(val, SUNXI_PWM_CTRL_REG);
    delay(1);
    print_pwm_reg();
}

void sunxi_pwm_set_mode(int mode) {
    int val = 0;
    val = readl(SUNXI_PWM_CTRL_REG);
    mode &= 1; //cover the mode to 0 or 1
    if (mode) { //pulse mode
        val |= (SUNXI_PWM_CH0_MS_MODE | SUNXI_PWM_CH0_PUL_START);
        pwmmode = 1;
    } else { //cycle mode
        val &= ~(SUNXI_PWM_CH0_MS_MODE);
        pwmmode = 0;
    }
    val |= (SUNXI_PWM_CH0_ACT_STA);
    if (wiringPiDebug)
        printf(">>function%s,no:%d,mode? :0x%x\n", __func__, __LINE__, val);
    writel(val, SUNXI_PWM_CTRL_REG);
    delay(1);
    print_pwm_reg();   
}

void sunxi_pwm_set_clk(int clk) {
    int val = 0;
    if (wiringPiDebug)
        printf(">>function%s,no:%d\n", __func__, __LINE__);
    // sunxi_pwm_set_enable(0);
    val = readl(SUNXI_PWM_CTRL_REG);
    if (wiringPiDebug)
        printf("read reg val: 0x%x\n", val);
    //clear clk to 0
    val &= 0xf801f0;
    val |= ((clk & 0xf) << 15); //todo check wether clk is invalid or not
    writel(val, SUNXI_PWM_CTRL_REG);
    sunxi_pwm_set_enable(1);
    if (wiringPiDebug)
        printf(">>function%s,no:%d,clk? :0x%x\n", __func__, __LINE__, val);
    delay(1);
    print_pwm_reg();
}

/**
 * ch0 and ch1 set the same,16 bit period and 16 bit act
 */
uint32_t sunxi_pwm_get_period(void) {
    uint32_t period_cys = 0;
    period_cys = readl(SUNXI_PWM_CH0_PERIOD); //get ch1 period_cys
    if (wiringPiDebug) {
        printf("periodcys: %d\n", period_cys);
    }
    period_cys &= 0xffff0000; //get period_cys
    period_cys = period_cys >> 16;
    if (wiringPiDebug)
        printf(">>func:%s,no:%d,period/range:%d", __func__, __LINE__, period_cys);
    delay(1);
    return period_cys;
}

uint32_t sunxi_pwm_get_act(void) {
    uint32_t period_act = 0;
    period_act = readl(SUNXI_PWM_CH0_PERIOD); //get ch1 period_cys
    period_act &= 0xffff; //get period_act
    if (wiringPiDebug)
        printf(">>func:%s,no:%d,period/range:%d", __func__, __LINE__, period_act);
    delay(1);
    return period_act;
}

void sunxi_pwm_set_period(int period_cys) {
    uint32_t val = 0;
    //all clear to 0
    if (wiringPiDebug)
        printf(">>func:%s no:%d\n", __func__, __LINE__);
    period_cys &= 0xffff; //set max period to 2^16
    period_cys = period_cys << 16;
    val = readl(SUNXI_PWM_CH0_PERIOD);
    if (wiringPiDebug)
        printf("read reg val: 0x%x\n", val);
    val &= 0x0000ffff;
    period_cys |= val;
    if (wiringPiDebug)
        printf("write reg val: 0x%x\n", period_cys);
    writel(period_cys, SUNXI_PWM_CH0_PERIOD);
    delay(1);
    val = readl(SUNXI_PWM_CH0_PERIOD);
    if (wiringPiDebug)
        printf("readback reg val: 0x%x\n", val);
    print_pwm_reg();
}

void sunxi_pwm_set_act(int act_cys) {
    uint32_t per0 = 0;
    //keep period the same, clear act_cys to 0 first
    if (wiringPiDebug)
        printf(">>func:%s no:%d\n", __func__, __LINE__);
    per0 = readl(SUNXI_PWM_CH0_PERIOD);
    if (wiringPiDebug)
        printf("read reg val: 0x%x\n", per0);
    per0 &= 0xffff0000;
    act_cys &= 0xffff;
    act_cys |= per0;
    if (wiringPiDebug)
        printf("write reg val: 0x%x\n", act_cys);
    writel(act_cys, SUNXI_PWM_CH0_PERIOD);
    delay(1);
    print_pwm_reg();
}

int sunxi_get_gpio_mode(int pin) {
    uint32_t regval = 0;
    int bank = pin >> 5;
    int index = pin - (bank << 5);
    int offset = ((index - ((index >> 3) << 3)) << 2);
    uint32_t reval = 0;
    uint32_t phyaddr = SUNXI_GPIO_BASE + (bank * 36) + ((index >> 3) << 2);
    if (wiringPiDebug)
        printf("func:%s pin:%d,  bank:%d index:%d phyaddr:0x%x\n", __func__, pin, bank, index, phyaddr);
    regval = readl(phyaddr);
    if (wiringPiDebug)
        printf("read reg val: 0x%x offset:%d  return: %d\n", regval, offset, reval);
    // reval=regval &(reval+(7 << offset));
    reval = (regval >> offset)&7;
    if (wiringPiDebug)
        printf("read reg val: 0x%x offset:%d  return: %d\n", regval, offset, reval);
    return reval;
}

void sunxi_set_gpio_mode(int pin, int mode) {
    uint32_t regval = 0;
    int bank = pin >> 5;
    int index = pin - (bank << 5);
    int offset = ((index - ((index >> 3) << 3)) << 2);
    uint32_t phyaddr = SUNXI_GPIO_BASE + (bank * 36) + ((index >> 3) << 2);
    if (wiringPiDebug)
        printf("func:%s pin:%d, MODE:%d bank:%d index:%d phyaddr:0x%x\n", __func__, pin, mode, bank, index, phyaddr);
    regval = readl(phyaddr);
    if (wiringPiDebug)
        printf("read reg val: 0x%x offset:%d\n", regval, offset);
    if (INPUT == mode) {
        regval &= ~(7 << offset);
        writel(regval, phyaddr);
        regval = readl(phyaddr);
        if (wiringPiDebug)
            printf("Input mode set over reg val: 0x%x\n", regval);
    } else if (OUTPUT == mode) {
        regval &= ~(7 << offset);
        regval |= (1 << offset);
        if (wiringPiDebug)
            printf("Out mode ready set val: 0x%x\n", regval);
        writel(regval, phyaddr);
        regval = readl(phyaddr);
        if (wiringPiDebug)
            printf("Out mode set over reg val: 0x%x\n", regval);
    }
    else if (PWM_OUTPUT == mode) {
        // set pin PWMx to pwm mode
        regval &= ~(7 << offset);
        regval |= (0x3 << offset);
        if (wiringPiDebug)
            printf(">>>>>line:%d PWM mode ready to set val: 0x%x\n", __LINE__, regval);
        writel(regval, phyaddr);
        delayMicroseconds(200);
        regval = readl(phyaddr);
        if (wiringPiDebug)
            printf("<<<<<PWM mode set over reg val: 0x%x\n", regval);
        //clear all reg
        writel(0, SUNXI_PWM_CTRL_REG);
        writel(0, SUNXI_PWM_CH0_PERIOD);

        //set default M:S to 1/2
        sunxi_pwm_set_period(1024);
        sunxi_pwm_set_act(512);
        pwmSetMode(PWM_MODE_MS);
        sunxi_pwm_set_clk(PWM_CLK_DIV_120); //default clk:24M/120
        delayMicroseconds(200);
    }

    return;
}

void sunxi_pullUpDnControl(int pin, int pud) {
    uint32_t regval = 0;
    int bank = pin >> 5;
    int index = pin - (bank << 5);
    int sub = index >> 4;
    int sub_index = index - 16 * sub;
    uint32_t phyaddr = SUNXI_GPIO_BASE + (bank * 36) + 0x1c + sub * 4; // +0x10 -> pullUpDn reg
    if (wiringPiDebug)
        printf("func:%s pin:%d,bank:%d index:%d sub:%d phyaddr:0x%x\n", __func__, pin, bank, index, sub, phyaddr);
    regval = readl(phyaddr);
    if (wiringPiDebug)
        printf("pullUpDn reg:0x%x, pud:0x%x sub_index:%d\n", regval, pud, sub_index);
    regval &= ~(3 << (sub_index << 1));
    regval |= (pud << (sub_index << 1));
    if (wiringPiDebug)
        printf("pullUpDn val ready to set:0x%x\n", regval);
    writel(regval, phyaddr);
    regval = readl(phyaddr);
    if (wiringPiDebug)
        printf("pullUpDn reg after set:0x%x  addr:0x%x\n", regval, phyaddr);
    delay(1);
    return;
}
/*end 2014.09.18*/

/*
 * wiringPiFailure:
 *	Fail. Or not.
 *********************************************************************************
 */

int wiringPiFailure(int fatal, const char *message, ...) {
    va_list argp;
    char buffer [1024];

    if (!fatal && wiringPiReturnCodes)
        return -1;

    va_start(argp, message);
    vsnprintf(buffer, 1023, message, argp);
    va_end(argp);

    fprintf(stderr, "%s", buffer);
    exit(EXIT_FAILURE);

    return 0;
}

/*
 * getAlt:
 *	Returns the ALT bits for a given port. Only really of-use
 *	for the gpio readall command (I think)
 *********************************************************************************
 */

int getAlt(int pin) {
    int alt;

    pin &= 63;

    alt = sunxi_get_gpio_mode(pin);
    return alt;

}

/*
 * pwmSetMode:
 *	Select the native "balanced" mode, or standard mark:space mode
 *********************************************************************************
 */

void pwmSetMode(int mode) {
    sunxi_pwm_set_mode(mode);
    return;
}

/*
 * pwmSetRange:
 *	Set the PWM range register. We set both range registers to the same
 *	value. If you want different in your own code, then write your own.
 *********************************************************************************
 */

void pwmSetRange(unsigned int range) {
    sunxi_pwm_set_period(range);
    return;
}

/*
 * pwmSetClock:
 *	Set/Change the PWM clock. Originally my code, but changed
 *	(for the better!) by Chris Hall, <chris@kchall.plus.com>
 *	after further study of the manual and testing with a 'scope
 *********************************************************************************
 */

void pwmSetClock(int divisor) {
    sunxi_pwm_set_clk(divisor);
    sunxi_pwm_set_enable(1);
    return;
}

/*
 * gpioClockSet:
 *	Set the freuency on a GPIO clock pin
 *********************************************************************************
 */

void gpioClockSet(int pin, int freq) {
    return;
}

/*
 *********************************************************************************
 * Core Functions
 *********************************************************************************
 */

/*
 * pinMode:
 *	Sets the mode of a pin to be input, output or PWM output
 *********************************************************************************
 */

void pinMode(int pin, int mode) {

    if (wiringPiDebug)
        printf("Func: %s, Line: %d,pin:%d,mode:%d\n", __func__, __LINE__, pin, mode);
    if (mode == INPUT) {
        sunxi_set_gpio_mode(pin, INPUT);
        wiringPinMode = INPUT;
        return;
    } else if (mode == OUTPUT) {
        sunxi_set_gpio_mode(pin, OUTPUT); //gootoomoon_set_mode
        wiringPinMode = OUTPUT;
        return;
    } else if (mode == PWM_OUTPUT) {
        if (pin != 5) {
            printf("the pin you choose doesn't support hardware PWM\n");
            printf("you can select wiringPi pin %d for PWM pin\n", 1);
            printf("or you can use it in softPwm mode\n");
            return;
        }
        //printf("you choose the hardware PWM:%d\n", 1);
        sunxi_set_gpio_mode(pin, PWM_OUTPUT);
        wiringPinMode = PWM_OUTPUT;
        return;
    } else
        return;
}

/*
 * pullUpDownCtrl:
 *	Control the internal pull-up/down resistors on a GPIO pin
 *	The Arduino only has pull-ups and these are enabled by writing 1
 *	to a port when in input mode - this paradigm doesn't quite apply
 *	here though.
 *********************************************************************************
 */

void pullUpDnControl(int pin, int pud) {
    pud = upDnConvert[pud];

    if (wiringPiDebug)
        printf("%s,%d,pin:%d\n", __func__, __LINE__, pin);

    if (-1 == pin) {
        printf("[%s:L%d] the pin:%d is invaild,please check it over!\n", __func__, __LINE__, pin);
        return;
    }
    pud &= 3;
    sunxi_pullUpDnControl(pin, pud);
}

/*
 * digitalRead:
 *	Read the value of a given Pin, returning HIGH or LOW
 *********************************************************************************
 */

int digitalRead(int pin) {
    uint32_t regval = 0;
    int bank = pin >> 5;
    int index = pin - (bank << 5);
    uint32_t phyaddr = SUNXI_GPIO_BASE + (bank * 36) + 0x10; // +0x10 -> data reg
    if (wiringPiDebug)
        printf("func:%s pin:%d,bank:%d index:%d phyaddr:0x%x\n", __func__, pin, bank, index, phyaddr);
    regval = readl(phyaddr);
    regval = regval >> index;
    regval &= 1;
    if (wiringPiDebug)
        printf("***** read reg val: 0x%x,bank:%d,index:%d,line:%d\n", regval, bank, index, __LINE__);
    return regval;
}

/*
 * digitalWrite:
 *	Set an output bit
 *********************************************************************************
 */

void digitalWrite(int pin, int value) {
    uint32_t regval = 0;
    int bank = pin >> 5;
    int index = pin - (bank << 5);
    uint32_t phyaddr = SUNXI_GPIO_BASE + (bank * 36) + 0x10; // +0x10 -> data reg
    if (wiringPiDebug)
        printf("func:%s pin:%d, value:%d bank:%d index:%d phyaddr:0x%x\n", __func__, pin, value, bank, index, phyaddr);
    regval = readl(phyaddr);
    if (wiringPiDebug)
        printf("befor write reg val: 0x%x,index:%d\n", regval, index);
    if (0 == value) {
        regval &= ~(1 << index);
        writel(regval, phyaddr);
        regval = readl(phyaddr);
        if (wiringPiDebug)
            printf("LOW val set over reg val: 0x%x\n", regval);
    } else {
        regval |= (1 << index);
        writel(regval, phyaddr);
        regval = readl(phyaddr);
        if (wiringPiDebug)
            printf("HIGH val set over reg val: 0x%x\n", regval);
    }
}

/*
 * pwmWrite:
 *	Set an output PWM value
 *********************************************************************************
 */

void pwmWrite(int pin, int value) {

    uint32_t a_val = 0;
    if (pwmmode == 1)//sycle
    {
        sunxi_pwm_set_mode(1);
    } else {
        //sunxi_pwm_set_mode(0);
    }
    if (pin != 5) {
        printf("please use soft pwmmode or choose PWM pin\n");
        return;
    }
    a_val = sunxi_pwm_get_period();
    if (wiringPiDebug)
        printf("==> no:%d period now is :%d,act_val to be set:%d\n", __LINE__, a_val, value);
    if (value - a_val > 0) {
        printf("val pwmWrite 0 <= X <= 1024\n");
        printf("Or you can set new range by yourself by pwmSetRange(range\n");
        return;
    }
    //if value changed chang it
    sunxi_pwm_set_enable(0);
    sunxi_pwm_set_act(value);
    sunxi_pwm_set_enable(1);
    if (wiringPiDebug)
        printf("this fun is ok now %s,%d\n", __func__, __LINE__);

    return;
}

/*
 * analogWrite:
 *	Write the analog value to the given Pin. 
 *	There is no on-board Pi analog hardware,
 *	so this needs to go to a new node.
 *********************************************************************************
 */

void analogWrite(int pin, int value) {
}

/*
 * pwmToneWrite:
 *	Pi Specific.
 *      Output the given frequency on the Pi's PWM pin
 *********************************************************************************
 */

void pwmToneWrite(int pin, int freq) {
    int range;

    if (freq == 0)
        pwmWrite(pin, 0); // Off
    else {
        range = 600000 / freq;
        pwmSetRange(range);
        pwmWrite(pin, freq / 2);
    }
}

/*
 * initialiseEpoch:
 *	Initialise our start-of-time variable to be the current unix
 *	time in milliseconds and microseconds.
 *********************************************************************************
 */

static void initialiseEpoch(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    epochMilli = (uint64_t) tv.tv_sec * (uint64_t) 1000 + (uint64_t) (tv.tv_usec / 1000);
    epochMicro = (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) (tv.tv_usec);
}

/*
 * delay:
 *	Wait for some number of milliseconds
 *********************************************************************************
 */

void delay(unsigned int howLong) {
    struct timespec sleeper, dummy;

    sleeper.tv_sec = (time_t) (howLong / 1000);
    sleeper.tv_nsec = (long) (howLong % 1000) * 1000000;

    nanosleep(&sleeper, &dummy);
}

/*
 * delayMicroseconds:
 *	This is somewhat intersting. It seems that on the Pi, a single call
 *	to nanosleep takes some 80 to 130 microseconds anyway, so while
 *	obeying the standards (may take longer), it's not always what we
 *	want!
 *
 *	So what I'll do now is if the delay is less than 100uS we'll do it
 *	in a hard loop, watching a built-in counter on the ARM chip. This is
 *	somewhat sub-optimal in that it uses 100% CPU, something not an issue
 *	in a microcontroller, but under a multi-tasking, multi-user OS, it's
 *	wastefull, however we've no real choice )-:
 *
 *      Plan B: It seems all might not be well with that plan, so changing it
 *      to use gettimeofday () and poll on that instead...
 *********************************************************************************
 */

void delayMicrosecondsHard(unsigned int howLong) {
    struct timeval tNow, tLong, tEnd;

    gettimeofday(&tNow, NULL);
    tLong.tv_sec = howLong / 1000000;
    tLong.tv_usec = howLong % 1000000;
    timeradd(&tNow, &tLong, &tEnd);

    while (timercmp(&tNow, &tEnd, <))
        gettimeofday(&tNow, NULL);
}

void delayMicroseconds(unsigned int howLong) {
    struct timespec sleeper;
    unsigned int uSecs = howLong % 1000000;
    unsigned int wSecs = howLong / 1000000;

    /**/ if (howLong == 0)
        return;
    else if (howLong < 100)
        delayMicrosecondsHard(howLong);
    else {
        sleeper.tv_sec = wSecs;
        sleeper.tv_nsec = (long) (uSecs * 1000L);
        nanosleep(&sleeper, NULL);
    }
}

/*
 * millis:
 *	Return a number of milliseconds as an unsigned int.
 *********************************************************************************
 */

unsigned int millis(void) {
    struct timeval tv;
    uint64_t now;

    gettimeofday(&tv, NULL);
    now = (uint64_t) tv.tv_sec * (uint64_t) 1000 + (uint64_t) (tv.tv_usec / 1000);

    return (uint32_t) (now - epochMilli);
}

/*
 * micros:
 *	Return a number of microseconds as an unsigned int.
 *********************************************************************************
 */

unsigned int micros(void) {
    struct timeval tv;
    uint64_t now;

    gettimeofday(&tv, NULL);
    now = (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) tv.tv_usec;

    return (uint32_t) (now - epochMicro);
}

/*
 * wiringPiSetup:
 *	Must be called once at the start of your program execution.
 *
 * Default setup: Initialises the system into wiringPi Pin mode and uses the
 *	memory mapped hardware directly.
 *
 * Changed now to revert to "gpio" mode if we're running on a Compute Module.
 *********************************************************************************
 */

int wiringPiSetup(int gpiomem) {
    int fd;
    //    int boardRev;
    if (getenv(ENV_DEBUG) != NULL)
        wiringPiDebug = TRUE;

    if (getenv(ENV_CODES) != NULL)
        wiringPiReturnCodes = TRUE;

    if (geteuid() != 0)
        (void)wiringPiFailure(WPI_FATAL, "wiringPiSetup: Must be root. (Did you forget sudo?)\n");

    if (wiringPiDebug)
        printf("wiringPi: wiringPiSetup called\n");

    // Open the master /dev/memory device

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
        return wiringPiFailure(WPI_ALMOST, "wiringPiSetup: Unable to open /dev/mem: %s\n", strerror(errno));

    // GPIO:
    // BLOCK SIZE * 2 increases range to include pwm addresses
    gpio = (uint32_t *) mmap(0, BLOCK_SIZE*10, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_BASE);
    if ((int32_t) gpio == -1)
        return wiringPiFailure(WPI_ALMOST, "wiringPiSetup: mmap (GPIO) failed: %s\n", strerror(errno));

    // PWM

    pwm = (uint32_t *) mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_PWM);
    if ((int32_t) pwm == -1)
        return wiringPiFailure(WPI_ALMOST, "wiringPiSetup: mmap (PWM) failed: %s\n", strerror(errno));

    // Clock control (needed for PWM)

    clk = (uint32_t *) mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CLOCK_BASE);
    if ((int32_t) clk == -1)
        return wiringPiFailure(WPI_ALMOST, "wiringPiSetup: mmap (CLOCK) failed: %s\n", strerror(errno));

    // The drive pads

    pads = (uint32_t *) mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_PADS);
    if ((int32_t) pads == -1)
        return wiringPiFailure(WPI_ALMOST, "wiringPiSetup: mmap (PADS) failed: %s\n", strerror(errno));



    initialiseEpoch();

    return 0;
}

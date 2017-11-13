/*
 * wiringPi:
 *	Arduino compatable (ish) Wiring library for the Raspberry Pi
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#ifndef	__WIRING_PI_H__
#define	__WIRING_PI_H__

// Handy defines

// Deprecated
#define	NUM_PINS	17

#define	WPI_MODE_PINS		 0
#define	WPI_MODE_GPIO		 1
#define	WPI_MODE_GPIO_SYS	 2
#define	WPI_MODE_PHYS		 3
#define	WPI_MODE_PIFACE		 4
#define	WPI_MODE_UNINITIALISED	-1

// Pin modes

#define	INPUT			 0
#define	OUTPUT			 1
#define	PWM_OUTPUT		 2
#define	GPIO_CLOCK		 3
#define	SOFT_PWM_OUTPUT		 4
#define	SOFT_TONE_OUTPUT	 5
#define	PWM_TONE_OUTPUT		 6


#define	LOW			 0
#define	HIGH			 1

// Pull up/down/none

#define	PUD_OFF			 0
#define	PUD_DOWN		 1
#define	PUD_UP			 2

// PWM

#define	PWM_MODE_MS		0
#define	PWM_MODE_BAL		1

// Interrupt levels

#define	INT_EDGE_SETUP		0
#define	INT_EDGE_FALLING	1
#define	INT_EDGE_RISING		2
#define	INT_EDGE_BOTH		3

// Pi model types and version numbers
//	Intended for the GPIO program Use at your own risk.

#define PI_MODEL_UNKNOWN 0
#define	PI_MODEL_M1	1

#define	PI_VERSION_UNKNOWN	0
#define	PI_VERSION_1		1
#define	PI_VERSION_1_1		2
#define	PI_VERSION_1_2		3
#define	PI_VERSION_2		4

#define	PI_MAKER_UNKNOWN	0
#define	PI_MAKER_EGOMAN		1
#define	PI_MAKER_SONY		2
#define	PI_MAKER_QISDA		3
#define   PI_MAKER_LEMAKER  4  //add for BananaPro by LeMaker team

//	Intended for the GPIO program Use at your own risk.

// Threads

#define	PI_THREAD(X)	void *X (void *dummy)

// Failure modes

#define	WPI_FATAL	(1==1)
#define	WPI_ALMOST	(1==2)


// Function prototypes
//	c++ wrappers thanks to a comment by Nick Lott
//	(and others on the Raspberry Pi forums)

#ifdef __cplusplus
extern "C" {
#endif

// Data


// Internal

extern int wiringPiFailure (int fatal, const char *message, ...) ;

// Core wiringPi functions

extern int  sunxi_init           (int gpiomem) ;

extern void sunxi_gpio_fsel      (uint8_t pin, uint8_t mode) ;
extern void sunxi_pullUpDnControl(int pin, int pud) ;
extern uint8_t sunxi_gpio_lev    (uint8_t pin) ;
extern void sunxi_gpio_write     (uint8_t pin, uint8_t on) ;
extern void analogWrite          (int pin, int value) ;

// PiFace specifics 
//	(Deprecated)

extern int  wiringPiSetupPiFace (void) ;
extern int  wiringPiSetupPiFaceForGpioProg (void) ;	// Don't use this - for gpio program only

// On-Board Raspberry Pi hardware specific stuff

extern int  piBoardRev          (void) ;
extern int  getAlt              (int pin) ;
extern void pwmToneWrite        (int pin, int freq) ;
extern void sunxi_pwm_set_mode  (uint8_t channel, uint8_t markspace, uint8_t enabled) ;
extern void sunxi_pwm_set_clock (uint32_t divisor);
extern void sunxi_pwm_set_range (uint32_t range) ;
extern void sunxi_pwm_set_data  (uint32_t data) ;
extern void gpioClockSet        (int pin, int freq) ;

// Threads

extern int  piThreadCreate      (void *(*fn)(void *)) ;
extern void piLock              (int key) ;
extern void piUnlock            (int key) ;

// Schedulling priority

extern int piHiPri (const int pri) ;

// Extras from arduino land

extern void         delay             (unsigned int howLong) ;
extern unsigned int millis            (void) ;
extern unsigned int micros            (void) ;

extern int sunxi_i2c_begin            (const int devId) ;
extern void sunxi_i2c_setSlaveAddress(uint8_t addr) ;
extern uint8_t sunxi_i2c_read(char* buf, uint32_t len) ;
extern uint8_t sunxi_i2c_write(char* buf, uint32_t len) ;

extern void sunxi_spi_begin(void) ;
extern int sunxi_spi_transfernb (char* tbuf, char* rbuf, uint32_t len) ;

#ifdef __cplusplus
}
#endif

#endif

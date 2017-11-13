/*
 * This implementation is based on WiringNP project
 * https://github.com/friendlyarm/WiringNP
 */

#ifndef	__SUNXI_PI_H__
#define	__SUNXI_PI_H__

// Function prototypes
#ifdef __cplusplus
extern "C" {
#endif

// Core GPIO functions
extern int     sunxi_init                (int gpiomem) ;
extern void    sunxi_gpio_fsel           (uint8_t pin, uint8_t mode) ;
extern void    sunxi_pullUpDnControl     (int pin, int pud) ;
extern uint8_t sunxi_gpio_lev            (uint8_t pin) ;
extern void    sunxi_gpio_write          (uint8_t pin, uint8_t on) ;

// PWM functions
extern void    sunxi_pwm_set_mode        (uint8_t channel, uint8_t markspace, uint8_t enabled) ;
extern void    sunxi_pwm_set_clock       (uint32_t divisor);
extern void    sunxi_pwm_set_range       (uint32_t range) ;
extern void    sunxi_pwm_set_data        (uint32_t data) ;

// i2c functions
extern int     sunxi_i2c_begin           (const int devId) ;
extern void    sunxi_i2c_setSlaveAddress (uint8_t addr) ;
extern uint8_t sunxi_i2c_read            (char* buf, uint32_t len) ;
extern uint8_t sunxi_i2c_write           (char* buf, uint32_t len) ;

// spi functions
extern void    sunxi_spi_begin           (void) ;
extern int     sunxi_spi_transfernb      (char* tbuf, char* rbuf, uint32_t len) ;

#ifdef __cplusplus
}
#endif

#endif

static uint8_t bcm2835_correct_order(uint8_t b)
{
}


uint32_t* bcm2835_regbase(uint8_t regbase)
{
	return 0;
}

void  bcm2835_set_debug(uint8_t d)
{
}

unsigned int bcm2835_version(void) 
{
    return BCM2835_VERSION;
}


uint32_t bcm2835_peri_read(volatile uint32_t* paddr)
{
	return 0;
}


uint32_t bcm2835_peri_read_nb(volatile uint32_t* paddr)
{
return 0;
}


void bcm2835_peri_write(volatile uint32_t* paddr, uint32_t value)
{

}


void bcm2835_peri_write_nb(volatile uint32_t* paddr, uint32_t value)
{

}


void bcm2835_peri_set_bits(volatile uint32_t* paddr, uint32_t value, uint32_t mask)
{
}


void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode)
{
}


void bcm2835_gpio_set(uint8_t pin)
{
}


void bcm2835_gpio_clr(uint8_t pin)
{
}


void bcm2835_gpio_set_multi(uint32_t mask)
{
}


void bcm2835_gpio_clr_multi(uint32_t mask)
{
}


uint8_t bcm2835_gpio_lev(uint8_t pin)
{
}


uint8_t bcm2835_gpio_eds(uint8_t pin)
{
}

uint32_t bcm2835_gpio_eds_multi(uint32_t mask)
{
}


void bcm2835_gpio_set_eds(uint8_t pin)
{
}

void bcm2835_gpio_set_eds_multi(uint32_t mask)
{
}


void bcm2835_gpio_ren(uint8_t pin)
{
}
void bcm2835_gpio_clr_ren(uint8_t pin)
{
}


void bcm2835_gpio_fen(uint8_t pin)
{
}
void bcm2835_gpio_clr_fen(uint8_t pin)
{
}

void bcm2835_gpio_hen(uint8_t pin)
{
}
void bcm2835_gpio_clr_hen(uint8_t pin)
{
}


void bcm2835_gpio_len(uint8_t pin)
{
}
void bcm2835_gpio_clr_len(uint8_t pin)
{
}


void bcm2835_gpio_aren(uint8_t pin)
{
}

void bcm2835_gpio_clr_aren(uint8_t pin)
{
}

void bcm2835_gpio_afen(uint8_t pin)
{
}

void bcm2835_gpio_clr_afen(uint8_t pin)
{
}

void bcm2835_gpio_pud(uint8_t pud)
{
}

void bcm2835_gpio_pudclk(uint8_t pin, uint8_t on)
{
}

uint32_t bcm2835_gpio_pad(uint8_t group)
{
    return 1;
}

void bcm2835_gpio_set_pad(uint8_t group, uint32_t control)
{
}

void bcm2835_delay(unsigned int millis)
{
}

void bcm2835_delayMicroseconds(uint64_t micros)
{
}

void bcm2835_gpio_write(uint8_t pin, uint8_t on)
{
}

void bcm2835_gpio_write_multi(uint32_t mask, uint8_t on)
{
}

void bcm2835_gpio_write_mask(uint32_t value, uint32_t mask)
{
}

void bcm2835_gpio_set_pud(uint8_t pin, uint8_t pud)
{
}


uint8_t bcm2835_gpio_get_pud(uint8_t pin)
{
    return 1;
}


int bcm2835_spi_begin(void)
{
    return 1; // OK
}

void bcm2835_spi_end(void)
{  
}

void bcm2835_spi_setBitOrder(uint8_t order)
{
}


void bcm2835_spi_setClockDivider(uint16_t divider)
{
}

void bcm2835_spi_set_speed_hz(uint32_t speed_hz)
{
}

void bcm2835_spi_setDataMode(uint8_t mode)
{
}

/* Writes (and reads) a single byte to SPI */
uint8_t bcm2835_spi_transfer(uint8_t value)
{
    return 1;
}


void bcm2835_spi_transfernb(char* tbuf, char* rbuf, uint32_t len)
{
}


void bcm2835_spi_writenb(const char* tbuf, uint32_t len)
{
}


void bcm2835_spi_transfern(char* buf, uint32_t len)
{
}

void bcm2835_spi_chipSelect(uint8_t cs)
{
}

void bcm2835_spi_setChipSelectPolarity(uint8_t cs, uint8_t active)
{
}

void bcm2835_spi_write(uint16_t data)
{
}

int bcm2835_aux_spi_begin(void)
{
    return 1; /* OK */
}

void bcm2835_aux_spi_end(void)
{
}

uint16_t bcm2835_aux_spi_CalcClockDivider(uint32_t speed_hz)
{
	return 1;
}

void bcm2835_aux_spi_setClockDivider(uint16_t divider)
{
}

void bcm2835_aux_spi_write(uint16_t data)
{
}

void bcm2835_aux_spi_writenb(const char *tbuf, uint32_t len) {
}

void bcm2835_aux_spi_transfernb(const char *tbuf, char *rbuf, uint32_t len) {
}

void bcm2835_aux_spi_transfern(char *buf, uint32_t len) {
}

int bcm2835_i2c_begin(void)
{
    return 1;
}

void bcm2835_i2c_end(void)
{
}

void bcm2835_i2c_setSlaveAddress(uint8_t addr)
{
}


void bcm2835_i2c_setClockDivider(uint16_t divider)
{
}

/* set I2C clock divider by means of a baudrate number */
void bcm2835_i2c_set_baudrate(uint32_t baudrate)
{
}

/* Writes an number of bytes to I2C */
uint8_t bcm2835_i2c_write(const char * buf, uint32_t len)
{
    return BCM2835_I2C_REASON_OK;
}

/* Read an number of bytes from I2C */
uint8_t bcm2835_i2c_read(char* buf, uint32_t len)
{
    return BCM2835_I2C_REASON_OK;
}

uint8_t bcm2835_i2c_read_register_rs(char* regaddr, char* buf, uint32_t len)
{   
    return BCM2835_I2C_REASON_OK;
}

uint8_t bcm2835_i2c_write_read_rs(char* cmds, uint32_t cmds_len, char* buf, uint32_t buf_len)
{   
    return BCM2835_I2C_REASON_OK;
}

uint64_t bcm2835_st_read(void)
{
}

void bcm2835_st_delay(uint64_t offset_micros, uint64_t micros)
{
}

void bcm2835_pwm_set_clock(uint32_t divisor)
{
}

void bcm2835_pwm_set_mode(uint8_t channel, uint8_t markspace, uint8_t enabled)
{
}

void bcm2835_pwm_set_range(uint8_t channel, uint32_t range)
{
}

void bcm2835_pwm_set_data(uint8_t channel, uint32_t data)
{
}

void *malloc_aligned(size_t size)
{
}

static void *mapmem(const char *msg, size_t size, int fd, off_t off)
{
}

static void unmapmem(void **pmem, size_t size)
{
}

/* Initialise this library. */
int bcm2835_init(int gpiomem)
{
	return 1;
}

/* Close this library and deallocate everything */
int bcm2835_close(void)
{
    return 1; /* Success */
}  

/// definition of in()/out()
#include <hw/inout.h>
/// for mmap_device_io()
#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>

#include "h3_i2c.h"

// TODO see all calls with DEBUG
//#define DEBUG 1

//#define SPTR_CAST(x)     (uintptr_t)&(x)  // 07.07.2025 go to .h

static uint8_t s_slave_address = 0;
static uint32_t s_current_baudrate = 0;

uint32_t read_reg(unsigned long long  port)
{
    uintptr_t base;

    if ( (base = mmap_device_io( 1, port )) == MAP_DEVICE_FAILED )
       {
           perror( "mmap_device_io()" );
       }

    return in32( base );
}

int write_reg (unsigned long long  port, uint32_t val)
{
    uintptr_t base;

    if ( (base = mmap_device_io( 1, port )) == MAP_DEVICE_FAILED )
    {
        perror( "mmap_device_io()" );
        return (-1);
    }

    out32( base, val );
    return (0);
}

static inline void _cc_write_reg(const uint32_t clk_n, const uint32_t clk_m) {
#ifdef DEBUG
    printf("\nInit Clock Register...\n");
#endif
    uint32_t value = read_reg( SPTR_CAST(EXT_I2C->CC));
#ifdef DEBUG
    printf("%s: clk_n = %d, clk_m = %d\n", __FUNCTION__, clk_n, clk_m);
#endif
    value &= _CAST(uint32_t)(~(CC_CLK_M | CC_CLK_N));
    value |= ((clk_n << CLK_N_SHIFT) | (clk_m << CLK_M_SHIFT));
    write_reg(SPTR_CAST(EXT_I2C->CC), value);

}

static void _set_clock(const uint32_t clk_in, const uint32_t sclk_req) {
    uint32_t clk_m = 0;
    uint32_t clk_n = 0;
    uint32_t _2_pow_clk_n = 1;
    const uint32_t src_clk = clk_in / 10;
    uint32_t divider = src_clk / sclk_req;
    uint32_t sclk_real = 0;

    assert(divider != 0);

    while (clk_n < (CLK_N_MASK + 1)) {
        /* (m+1)*2^n = divider -->m = divider/2^n -1 */
        clk_m = (divider / _2_pow_clk_n) - 1;
        /* clk_m = (divider >> (_2_pow_clk_n>>1))-1 */

        while (clk_m < (CLK_M_MASK + 1)) {
            sclk_real = src_clk / (clk_m + 1) / _2_pow_clk_n; /* src_clk/((m+1)*2^n) */

            if (sclk_real <= sclk_req) {
                _cc_write_reg(clk_n, clk_m);
                return;
            } else {
                clk_m++;
            }
        }

        clk_n++;
        _2_pow_clk_n *= 2;
    }

    _cc_write_reg(clk_n, clk_m);

    return;
}

void h3_i2c_set_baudrate(const uint32_t nBaudrate) {
    assert(nBaudrate <= H3_I2C_FULL_SPEED);

    if (__builtin_expect((s_current_baudrate != nBaudrate), 0)) {
        s_current_baudrate = nBaudrate;
        _set_clock(H3_F_24M, nBaudrate);
    }
}

void h3_i2c_set_slave_address(const uint8_t nAddress) {
    s_slave_address = nAddress;
}

static int32_t _stop() {
    int32_t time = TIMEOUT;
    uint32_t tmp_val;

#ifdef DEBUG
    printf("\n_stop() now...\n");
#endif

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
    tmp_val |= (0x01 << 4);                         // Master Mode Stop (bit #4)
    write_reg(SPTR_CAST(EXT_I2C->CTL), tmp_val);

    //while ((time--) && (EXT_I2C->CTL & 0x10))
    while ((time--) && (read_reg(SPTR_CAST(EXT_I2C->CTL)) & 0x10))       // wait when Stop bit is down
        /*printf("wait stop bit is down\n")*/;

    if (time <= 0) {
        return -H3_I2C_NOK_TOUT;
    }

    time = TIMEOUT;
    //while ((time--) && (EXT_I2C->STAT != STAT_READY))
    while ((time--) && (read_reg(SPTR_CAST(EXT_I2C->STAT)) != STAT_READY))
        ;

    if (time <= 0) {
        return -H3_I2C_NOK_TOUT;
    }
#ifdef DEBUG
    printf ("\nIn %s STAT is:\n", __FUNCTION__);
    //print_reg(SPTR_CAST(EXT_I2C->STAT), 1);
    printf("_stop() is OK\n");
#endif

    return H3_I2C_OK;
}

static int32_t _sendstart() {
    int32_t time = 0xff;
    uint32_t tmp_val;

#ifdef DEBUG
    printf ("\nEnter to _sendstart()\n");
#endif

    write_reg(SPTR_CAST(EXT_I2C->EFR), 0x0);          // Data Byte to be written after read command
#ifdef DEBUG
    printf ("\nEFR->0 is OK, Soft Reset TWI0 now... \n");
#endif
    write_reg(SPTR_CAST(EXT_I2C->SRST), 1);
//    printf ("SRST->1 is OK\n");
    usleep(100);    // TODO - why sleep??

#ifdef DEBUG
    printf("Before sendstart() CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
    printf("Before sendstart() STAT->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->STAT)) );
//    tmp_val = (read_reg(SPTR_CAST(EXT_I2C->CTL)) & 0x08);
//    printf("res = 0x%0X\n", tmp_val);
#endif

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
    tmp_val  |= 0x20;                               // Master Mode start (bit #5)
    //tmp_val &= ~CTL_INT_FLAG;
    //printf("Write to EXT_I2C->CTL 0x%0X\n", tmp_val);

    write_reg(SPTR_CAST(EXT_I2C->CTL), tmp_val);

//    tmp_val = (read_reg(SPTR_CAST(EXT_I2C->CTL)) & 0x08);
//    printf("res = 0x%0X\n", tmp_val);

    while ((time--) && (!(read_reg(SPTR_CAST(EXT_I2C->CTL)) & 0x08)))   // wait INT_FLAG
        printf("in _sendstart() wait INT_FLAG time = 0x%0X\n", time)
        ;

    if (time <= 0) {
        return -H3_I2C_NOK_TOUT;
    }

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->STAT));

    if (tmp_val != STAT_START_TRANSMIT) {
        return -STAT_START_TRANSMIT;
    }
#ifdef DEBUG
    printf("\nAfter _sendstart() CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
    printf("After _sendstart() STAT->0x%0X\n", tmp_val );
    printf("\n%s is OK\n", __FUNCTION__);
#endif

    // ****** try to unset INT
    tmp_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
    tmp_val &= ~CTL_INT_FLAG;
    //printf("Write to EXT_I2C->CTL 0x%0X\n", tmp_val);
    write_reg(SPTR_CAST(EXT_I2C->CTL), tmp_val);
    // ******

    usleep(200);    // TODO - why sleep??
    return H3_I2C_OK;
}

static int32_t _sendrestart() {
    int32_t time = 0xff;
    uint32_t tmp_val;

#ifdef DEBUG
    printf ("\nEnter to _sendrestart()\n");
#endif


#ifdef DEBUG
    printf("Before _sendrestart() CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
    printf("Before _sendrestart() STAT->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->STAT)) );
//    tmp_val = (read_reg(SPTR_CAST(EXT_I2C->CTL)) & 0x08);
//    printf("res = 0x%0X\n", tmp_val);
#endif

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
    tmp_val  |= 0x20;                               // Master Mode start (bit #5)
    //tmp_val &= ~CTL_INT_FLAG;
    //printf("Write to EXT_I2C->CTL 0x%0X\n", tmp_val);

    write_reg(SPTR_CAST(EXT_I2C->CTL), tmp_val);

//    tmp_val = (read_reg(SPTR_CAST(EXT_I2C->CTL)) & 0x08);
//    printf("res = 0x%0X\n", tmp_val);

    while ((time--) && (!(read_reg(SPTR_CAST(EXT_I2C->CTL)) & 0x08)))   // wait INT_FLAG
        printf("in _sendrestart() wait INT_FLAG time = 0x%0X\n", time)
        ;

    if (time <= 0) {
        return -H3_I2C_NOK_TOUT;
    }

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->STAT));

#ifdef DEBUG
    printf("\nAfter _sendrestart() CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
    printf("After _sendrestart() STAT->0x%0X\n", tmp_val );
    printf("\n%s is OK\n", __FUNCTION__);
#endif

    if (tmp_val != STAT_RESTART_TRANSMIT) {
        return -STAT_RESTART_TRANSMIT;
    }


    return H3_I2C_OK;
}

static int32_t _sendslaveaddr(uint32_t mode) {
    int32_t i = 0, time = TIMEOUT;
    uint32_t tmp_val;

    mode &= 1;

#ifdef DEBUG
    printf("\n_sendslave() started...\n");
    printf("\nBefore _sendslave() CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
#endif

    tmp_val = _CAST(uint32_t)(s_slave_address << 1) | mode;
#ifdef DEBUG
    printf(" address to be write - 0x%0X\n", tmp_val);
#endif
    write_reg(SPTR_CAST(EXT_I2C->DATA), tmp_val);
//    printf("\nIn %s DATA is:\n", __FUNCTION__);
//    print_reg(SPTR_CAST(EXT_I2C->DATA), 1);


    tmp_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
    tmp_val |= (0x01 << 3);                              // INT_FLAG ^
    //tmp_val |= CTL_A_ACK;
    write_reg(SPTR_CAST(EXT_I2C->CTL), tmp_val);

    //while ((time--) && (!(EXT_I2C->CTL & 0x08)))
    while ((time--) && (!(read_reg(SPTR_CAST(EXT_I2C->CTL)) & 0x08)))   // wait INT_FLAG
        ;

    if (time <= 0) {
        return -H3_I2C_NOK_TOUT;
    }

    //printf("***2***CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
    tmp_val = read_reg(SPTR_CAST(EXT_I2C->STAT));
#ifdef DEBUG
    printf("\nAfter _sendslave() CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
    printf("STAT->0x%0X\n", tmp_val);
#endif

    if (mode == I2C_MODE_WRITE) {
        if (tmp_val != STAT_ADDRWRITE_ACK) {
            return -STAT_ADDRWRITE_ACK;
        }
    } else {
        if (tmp_val != STAT_ADDRREAD_ACK) {
            return -STAT_ADDRREAD_ACK;
        }
    }
#ifdef DEBUG
    printf("%s() is OK\n", __FUNCTION__);
#endif
    return H3_I2C_OK;
}

static int _read(char *buffer, int len, uint8_t reg_code) {
    int i=0, ret, ret0 = -1;
    int32_t time = 0xffff;
    uint32_t tmp_val;

    uint8_t byte1, byte2;

    ret = _sendstart();
    if (ret) {
        goto i2c_read_err_occur;
    }


    ret = _sendslaveaddr(I2C_MODE_WRITE);
#ifdef DEBUG
    printf ("\n_sendslaveaddr return %d\n", ret);
    printf("+CTL->0x%0X\n\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
#endif

    if (ret) {
        goto i2c_read_err_occur;
    }

    write_reg(SPTR_CAST(EXT_I2C->DATA), reg_code);

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
    tmp_val |= (0x01 << 3);                              // ^^^ INT_FLAG
    write_reg(SPTR_CAST(EXT_I2C->CTL), tmp_val);

    while ((time--) && ((read_reg(SPTR_CAST(EXT_I2C->STAT)) != 0x28)))
        /*printf(" wait STAT 0x28 time = 0x%0X\n", time)*/;


    if (time <= 0)
    {
        goto i2c_read_err_occur;
    }
    else
    {
#ifdef DEBUG
    printf ("\n Send 0x%0X (Reg Addr) is OK\n\n", reg_code);
    printf("+STAT->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->STAT)) );
    printf("+CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
#endif
    }

    ret = _sendrestart();
    if (ret) {
        goto i2c_read_err_occur;
    }

#ifdef DEBUG
    printf("_restart() is OK\n");
    printf("STAT->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->STAT)) );
#endif

    ret = _sendslaveaddr(I2C_MODE_READ);

    if (ret) {
        goto i2c_read_err_occur;
    }

#ifdef DEBUG
    printf("++STAT->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->STAT)) );
    printf("++CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
#endif


    while ((time--) && (!(read_reg(SPTR_CAST(EXT_I2C->CTL)) & 0x08)))   // wait INT_FLAG
        printf("*");
#ifdef DEBUG
    // Receiving data
    printf("===============\n");
    printf("Receiving data:\n");
    printf("===============\n");
#endif
    time = 0xffff;
    tmp_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
    tmp_val |= CTL_A_ACK;
    tmp_val |= CTL_INT_FLAG;
    write_reg(SPTR_CAST(EXT_I2C->CTL), tmp_val);

    while ((time--) && ((read_reg(SPTR_CAST(EXT_I2C->STAT)) != 0x50)))
        ;

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->DATA));
    byte1 = (uint8_t)tmp_val;
    *buffer = (char)tmp_val;

#ifdef DEBUG
    printf (" Data(MSB) = 0x%0X\n", /*tmp_val*/byte1);
    printf("Measurement time in proc tick = %d\n", 0xffff-time);
    printf("++STAT->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->STAT)) );
    printf("++CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
#endif
    tmp_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
    tmp_val |= CTL_A_ACK;
    tmp_val |= CTL_INT_FLAG;
    write_reg(SPTR_CAST(EXT_I2C->CTL), tmp_val);

    while ((time--) && ((read_reg(SPTR_CAST(EXT_I2C->STAT)) != 0x50)))
        ;

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->DATA));
    byte2 = (uint8_t)tmp_val;
    buffer[1] = (char)tmp_val;

#ifdef DEBUG
    printf (" Data(LSB) = 0x%0X\n", /*tmp_val*/byte2);
    printf("Measurement time in proc tick = %d\n", 0xffff-time);
    printf("++STAT->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->STAT)) );
    printf("++CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
#endif

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
    tmp_val &= ~CTL_A_ACK;
    tmp_val |= CTL_INT_FLAG;
    write_reg(SPTR_CAST(EXT_I2C->CTL), tmp_val);

    while ((time--) && ((read_reg(SPTR_CAST(EXT_I2C->STAT)) != 0x58)))
        ;

    tmp_val = read_reg(SPTR_CAST(EXT_I2C->DATA));
    buffer[2] = (char)tmp_val;
#ifdef DEBUG
    printf (" CHECKSUM  = 0x%0X\n", tmp_val);
    printf("Measurement time in proc tick = %d\n", 0xffff-time);
    printf("++STAT->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->STAT)) );
    printf("++CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
#endif

#ifdef DEBUG
    printf("++STAT->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->STAT)) );
    printf("++CTL->0x%0X\n", read_reg(SPTR_CAST(EXT_I2C->CTL)) );
#endif

    ret0 = 0;

    i2c_read_err_occur: _stop();

    return ret0;
}

uint8_t h3_i2c_read(char *buffer, uint32_t data_length, uint8_t reg_code) {

    const auto ret = _read(buffer, _CAST(int)(data_length), reg_code);

#ifdef DEBUG
    if (ret) {
        printf("%s ret=%d\n", __FUNCTION__, ret);
    }
#endif
    return _CAST(uint8_t)(-ret);
}

/// definition of in()/out()
#include <hw/inout.h>
/// for mmap_device_io()
#include <sys/mman.h>
#include <assert.h>

#include "h3_i2c.h"

//#define DEBUG 1

#define SPTR_CAST(x)     (uintptr_t)&(x)

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

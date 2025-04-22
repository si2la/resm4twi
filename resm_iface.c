#include "resm_iface.h"

uint8_t verbose = 0;
// number of interface: twi0, twi1 etc.
uint8_t twi_num = 0;

int twi_version_info(i2c_libversion_t *version)
{
    version->major = 0;
    version->minor = 0;
    version->revision = 1;

    return 0;
}

void *
twi_init(int argc, char *argv[])
{
    twi_dev_t   *dev;
    int option;

    // is IO operations allow
    if ( ThreadCtl( _NTO_TCTL_IO, 0 ) == -1 )
    {
        fprintf(stderr, "You must be root!" );
        return NULL;
    }

    dev = malloc(sizeof(twi_dev_t));
    if (!dev) return NULL;

    dev->speed = 100000;
    dev->slave_addr = 0x0;
    dev->twi_num = 0x0;

    while ( (option = getopt( argc, argv, "d:a:s:v" )) != -1 )
    {
        switch ( option )
        {
            case 'd':
                //device_name = optarg;
                dev->twi_num = strtol( optarg, NULL, 0 );
                break;

            case 'a':
                dev->slave_addr = strtol( optarg, NULL, 0 );
                break;

            case 's':
                dev->speed = strtol( optarg, NULL, 0 );
                break;

            case 'v':
                verbose = 1;
                optv = 1;       // extern fo resmgr_twi module
                break;

//            case 'i': // not working, why - i don't know...
//                dev->twi_num = strtol( optarg, NULL, 0 );
//                break;

            default:
                return NULL;
        }
    }

    if (dev->slave_addr == 0) goto fail_free_dev;

    if( verbose )
    {
        fprintf(stdout, "slave_addr = %d\n", dev->slave_addr);
        fprintf(stdout, "speed = %d\n", dev->speed);
        fprintf(stdout, "dev_num = %d\n", dev->twi_num);
        fflush;
    }

    // init i2c (twi) interface
    // option -i is needed -> to work with choosed interface
    if ( dev->twi_num > 2 )  dev->twi_num = 0;

    uint32_t cfg_reg_val = 0;
    switch ( dev->twi_num )
    {
        case 0:
        // TWI0
            // read config reg for pins & touch some bits for TWI using
            cfg_reg_val = read_reg(H3_PIO_BASE + H3_PA_CFG1_REG);
            // 12 bit down
            cfg_reg_val &= ~( 0x01 << 12 );
            // 13 bit up
            cfg_reg_val |=  ( 0x01 << 13 );
            // 14 bit down
            cfg_reg_val &= ~( 0x01 << 14 );
            // 16 bit down
            cfg_reg_val &= ~( 0x01 << 16 );
            // 17 bit up
            cfg_reg_val |=  ( 0x01 << 17 );
            // 18 bit down
            cfg_reg_val &= ~( 0x01 << 18 );
            write_reg(H3_PIO_BASE + H3_PA_CFG1_REG, cfg_reg_val);

            // now set TWI0 gating
            cfg_reg_val = read_reg(H3_CCU_BASE + H3_BUS_CLK_GATING_REG3);
            cfg_reg_val |= 0x01;
            write_reg(H3_CCU_BASE + H3_BUS_CLK_GATING_REG3, cfg_reg_val);

            // and Bus soft reset
            cfg_reg_val = read_reg(H3_CCU_BASE + H3_BUS_SOFT_RST_REG4);
            cfg_reg_val |= 0x01;
            write_reg(H3_CCU_BASE + H3_BUS_SOFT_RST_REG4, cfg_reg_val);

            break;

        case 1:
            break;

        case 2:
            break;

        default:
            break;
    }

    return dev;

fail_free_dev:
    free(dev);
    return NULL;
}

int
i2c_master_getfuncs(i2c_master_funcs_t *funcs, int tabsize)
{
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            version_info, twi_version_info, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            init, twi_init, tabsize);

    return 0;
}


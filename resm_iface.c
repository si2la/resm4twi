#include "resm_iface.h"

uint8_t verbose = 0;
// number of interface: twi0, twi1, twi2
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

    // is IO operations allow?
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

    if (dev->slave_addr == 0) goto fail_dev;

    if( verbose )
    {
        fprintf(stdout, "Slave_addr = %d\n", dev->slave_addr);
        fprintf(stdout, "Speed = %d\n", dev->speed);
        fprintf(stdout, "Dev_num = %d\n", dev->twi_num);
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

            if (verbose) fprintf(stdout, "Bus Cfg Reg value = 0x%0X\n", cfg_reg_val);
            if (write_reg(H3_PIO_BASE + H3_PA_CFG1_REG, cfg_reg_val)) goto fail_dev;
            else
            {
                if (verbose) fprintf(stdout, "Write - OK!\n");
            }

            // now set TWI0 gating
            cfg_reg_val = read_reg(H3_CCU_BASE + H3_BUS_CLK_GATING_REG3);
            cfg_reg_val |= 0x01;

            if (verbose) fprintf(stdout, "Bus CLK gating Reg value = 0x%0X\n", cfg_reg_val);
            if (write_reg(H3_CCU_BASE + H3_BUS_CLK_GATING_REG3, cfg_reg_val)) goto fail_dev;
            else
            {
                if (verbose) fprintf(stdout, "Write - OK!\n");
            }

            // and Bus soft reset TWI0
            cfg_reg_val = read_reg(H3_CCU_BASE + H3_BUS_SOFT_RST_REG4);
            cfg_reg_val |= 0x01;

            if (verbose) fprintf(stdout, "Bus Soft reset Reg value = 0x%0X\n", cfg_reg_val);
            if (write_reg(H3_CCU_BASE + H3_BUS_SOFT_RST_REG4, cfg_reg_val)) goto fail_dev;
            else
            {
                if (verbose) fprintf(stdout, "Write - OK!\n");
            }

            // check right init state
            if (verbose)
            {
                cfg_reg_val = read_reg(SPTR_CAST(EXT_I2C->CTL));
                fprintf(stdout, "TWI0 CTL Reg value = 0x%0X\n", cfg_reg_val);
            }

            // 0xC0 - (INT_EN + BUS_EN) see page 444 of Allwinner_H3_Datasheet_V1.2
            if ( write_reg(SPTR_CAST(EXT_I2C->CTL), 0xC0) ) goto fail_dev;

            int i = 0;
            while ( ++i < 10 && (read_reg(SPTR_CAST(EXT_I2C->CTL)) != 0xC0)) printf("wait CTL to set 0xC0...");
            if (i >= 9) goto fail_dev;


            // end of TWI0 initialization

            break;

        case 1:
            break;

        case 2:
            break;

        default:
            break;
    }

    h3_i2c_set_baudrate(dev->speed);
    h3_i2c_set_slave_address(dev->slave_addr);

    return dev;

fail_dev:
    free(dev);
    return NULL;
}

int
twi_ctl(void *hdl, int cmd, void *msg, int msglen,
             int *nbytes, int *info)
{
    twi_dev_t   *dev = hdl;
    int ret;

//    printf("slave_addr = %d\n", dev->slave_addr);
//    printf("speed = %d\n", dev->speed);
//    printf("dev_num = %d\n", dev->twi_num);

    // TODO - may be add buffer to dev structure?

    // now send request (register 0xE3 or 0xE5)
    uint8_t * buffer = msg;

    printf("TWI_RM: Quering 0x%0X register from device with addr 0x%0X...\n", cmd, dev->slave_addr);
    if ( h3_i2c_read(buffer, 3, cmd) )
    {
        printf("TWI_RM: Error occured\n");
        return I2C_STATUS_ERROR;
    }

    ret = I2C_STATUS_DONE;

    return ret;
}

int
i2c_master_getfuncs(i2c_master_funcs_t *funcs, int tabsize)
{
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            version_info, twi_version_info, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            init, twi_init, tabsize);
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            ctl, twi_ctl, tabsize);

    return 0;
}


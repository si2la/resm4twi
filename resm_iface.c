#include "resm_iface.h"

_uint8 verbose = 0;

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

    while ( (option = getopt( argc, argv, "a:s:v" )) != -1 )
    {
        switch ( option )
        {
//            case 'd':
//                device_name = optarg;
//                break;

            case 'a':
                dev->slave_addr = strtol( optarg, NULL, 0 );
                break;

            case 's':
                dev->speed = strtol( optarg, NULL, 0 );
                break;

            case 'v':
                verbose = 1;
                optv = 1;
                break;

            default:
                return NULL;
        }
    }

    if (dev->slave_addr == 0) goto fail_free_dev;

    if( verbose )
    {
        fprintf(stdout, "slave_addr = %d\n", dev->slave_addr);
        fprintf(stdout, "speed = %d\n", dev->speed);
        fflush;
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


#include "resm_iface.h"

int twi_version_info(i2c_libversion_t *version)
{
    version->major = 0;
    version->minor = 0;
    version->revision = 1;
}

int
i2c_master_getfuncs(i2c_master_funcs_t *funcs, int tabsize)
{
    I2C_ADD_FUNC(i2c_master_funcs_t, funcs,
            version_info, twi_version_info, tabsize);

    return 0;
}


#ifndef RESM_IFACE_H
#define RESM_IFACE_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/neutrino.h>

#include <hw/i2c.h>

#include "h3_i2c.h"


int optv;

typedef struct _twi_dev
{
    unsigned int    speed;
    unsigned int    slave_addr;
    uint8_t         *buf;
} twi_dev_t;

int twi_version_info(i2c_libversion_t *version);
void * twi_init(int argc, char *argv[]);

#endif // RESM_IFACE_H

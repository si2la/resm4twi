#ifndef RESM_IFACE_H
#define RESM_IFACE_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/neutrino.h>

// for stty
#include <sys/dcmd_chr.h>
#include <termios.h>

#include <hw/i2c.h>

#include "h3_i2c.h"


int optv;       // global option -v (verbose)

typedef struct _twi_dev
{
    unsigned int    speed;
    unsigned int    slave_addr;
    uint8_t         twi_num;
    uint8_t         *buf;
} twi_dev_t;

typedef union _my_devctl_msg {
    int tx;     /* Filled by client on send */
    int rx;     /* Filled by server on reply */
} data_t;

int twi_version_info(i2c_libversion_t *version);
void * twi_init(int argc, char *argv[]);

#endif // RESM_IFACE_H

#ifndef H3_I2C_H
#define H3_I2C_H

#include <stdint.h>

# define _CAST(x)	(x)

#define MEGABYTE				0x100000
#define H3_F_24M				24000000

#define H3_TWI_BASE				0x01C2AC00

#define     __I		volatile		///< defines 'read only' permissions
#define		__O		volatile		///< defines 'write only' permissions
#define		__IO	volatile		///< defines 'read / write' permissions

typedef struct T_H3_TWI {
    __IO uint32_t ADDR;				///< 0x00 TWI Slave Address
    __IO uint32_t XADDR;			///< 0x04 TWI Extended slave Address
    __IO uint32_t DATA;				///< 0x08
    __IO uint32_t CTL;				///< 0x0C
    __I  uint32_t STAT;				///< 0x10 TWI Status register
    __IO uint32_t CC;				///< 0x14 TWI Clock Control Register
    __IO uint32_t SRST;				///< 0x18 TWI Software reset
    __IO uint32_t EFR;				///< 0x1C TWI Enhance Feature Register
    __IO uint32_t LCR;				///< 0x20 TWI Line Control register
    __IO uint32_t DVFS;				///< 0x24
} H3_TWI_TypeDef;

/* http://linux-sunxi.org/PRCM */

#define H3_TWI0_BASE			(H3_TWI_BASE + (0 * 0x400))
#define H3_TWI1_BASE			(H3_TWI_BASE + (1 * 0x400))
#define H3_TWI2_BASE			(H3_TWI_BASE + (2 * 0x400))

#define EXT_I2C_BASE            H3_TWI0_BASE

#define EXT_I2C_NUMBER			((EXT_I2C_BASE - H3_TWI_BASE) / 0x400)
#define EXT_I2C					(_CAST(H3_TWI_TypeDef *)(EXT_I2C_BASE))

typedef enum H3_I2C_BAUDRATE {
    H3_I2C_NORMAL_SPEED = 100000,
    H3_I2C_FULL_SPEED = 400000
} h3_i2c_baudrate_t;

typedef enum H3_I2C_RC {
    H3_I2C_OK = 0,
    H3_I2C_NOK = 1,
    H3_I2C_NACK = 2,
    H3_I2C_NOK_LA = 3,
    H3_I2C_NOK_TOUT = 4
} h3_i2c_rc_t;

typedef enum I2C_MODE {
    I2C_MODE_WRITE = 0,
    I2C_MODE_READ
} i2c_mode_t;

#define STAT_BUS_ERROR			0x00		///< Bus error
#define STAT_START_TRANSMIT     0x08		///< START condition transmitted
#define STAT_RESTART_TRANSMIT   0x10		///< Repeated START condition transmitted
#define STAT_ADDRWRITE_ACK	   	0x18		///< Address+Write bit transmitted, ACK received
#define STAT_DATAWRITE_ACK		0x28		///< Data transmitted in master mode, ACK received
#define STAT_ADDRREAD_ACK	   	0x40		///< Address+Read bit transmitted, ACK received
#define STAT_DATAREAD_ACK		0x50		///< Data byte received in master mode, ACK transmitted
#define STAT_DATAREAD_NACK	   	0x58		///< Data byte received in master mode, not ACK transmitted
#define STAT_READY			   	0xf8		///< No relevant status information, INT_FLAG=0

#define CTL_A_ACK				(1U << 2)	///< Assert Acknowledge
#define CTL_INT_FLAG			(1U << 3)	///< Interrupt Flag
#define CTL_M_STP				(1U << 4)	///< Master Mode Stop
#define CTL_M_STA				(1U << 5)	///< Master Mode Start
#define CTL_BUS_EN				(1U << 6)	///< TWI Bus Enable
#define CTL_INT_EN				(1U << 7)	///< Interrupt Enable

#define CC_CLK_N				(0x7 << 0)
#define CLK_N_SHIFT             0
#define CLK_N_MASK              0x7
#define CC_CLK_M				(0xF << 3)
#define CLK_M_SHIFT             3
#define CLK_M_MASK              0xF
#define LCR_SDA_EN              (0x01<<0) 	/* SDA line state control enable ,1:enable; 0:disable */
#define TWI_LCR_SDA_CTL         (0x01<<1) 	/* SDA line state control bit, 1:high level; 0:low level */
#define LCR_SCL_EN              (0x01<<2) 	/* SCL line state control enable ,1:enable; 0:disable */
#define TWI_LCR_SCL_CTL         (0x01<<3) 	/* SCL line state control bit, 1:high level; 0:low level */
#define TWI_LCR_SDA_STATE_MASK  (0x01<<4)   /* current state of SDA,readonly bit */
#define TWI_LCR_SCL_STATE_MASK  (0x01<<5)   /* current state of SCL,readonly bit */

#define TIMEOUT			0xffff

void h3_i2c_set_baudrate(const uint32_t nBaudrate);
void h3_i2c_set_slave_address(const uint8_t nAddress);
uint8_t h3_i2c_read(char *, uint32_t, uint8_t);
int htu21d_i2c_connected(const uint8_t nAddress, const uint32_t nBaudrate);

#endif // H3_I2C_H

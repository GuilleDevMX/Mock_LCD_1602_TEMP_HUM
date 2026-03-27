#ifndef I2C_HAL_H
#define I2C_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "system.h"

/* =========================================================================
 * REGISTROS DEL INTEL FPGA I2C IP
 * ========================================================================= */
#define I2C_TFR_CMD_REG      (*(volatile uint32_t *)(I2C_BASE + 0x00U))
#define I2C_RX_DATA_REG      (*(volatile uint32_t *)(I2C_BASE + 0x04U))
#define I2C_CTRL_REG         (*(volatile uint32_t *)(I2C_BASE + 0x08U))
#define I2C_STATUS_REG       (*(volatile uint32_t *)(I2C_BASE + 0x14U))
#define I2C_TFR_CMD_FIFO_LVL (*(volatile uint32_t *)(I2C_BASE + 0x18U))

/* Bits de Control y Estado */
#define I2C_TFR_CMD_STA      (1U << 9)
#define I2C_TFR_CMD_STO      (1U << 10)
#define I2C_TFR_CMD_RW_D     (1U << 8)
#define I2C_CTRL_EN          (1U << 0)
#define I2C_STATUS_CORE_BUSY (1U << 0)
#define I2C_STATUS_ARB_LOST  (1U << 1)
#define I2C_STATUS_NACK_DET  (1U << 2)

void I2C_Init(void);
bool I2C_Is_Busy(void);
bool I2C_Write_FIFO(uint8_t dev_addr, const uint8_t *data, uint8_t len);
bool I2C_Read_Request(uint8_t dev_addr, uint8_t len);
bool I2C_Read_Rx_FIFO(uint8_t *buffer, uint8_t len);

#endif /* I2C_HAL_H */
#ifndef I2C_HAL_H
#define I2C_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "system.h" /* Para BASE_I2C */

/* =========================================================================
 * REGISTROS DEL INTEL FPGA I2C IP (Espaciado de 32-bits)
 * ========================================================================= */
#define I2C_TFR_CMD_REG      (*(volatile uint32_t *)(BASE_I2C + 0x00U))
#define I2C_RX_DATA_REG      (*(volatile uint32_t *)(BASE_I2C + 0x04U))
#define I2C_CTRL_REG         (*(volatile uint32_t *)(BASE_I2C + 0x08U))
#define I2C_STATUS_REG       (*(volatile uint32_t *)(BASE_I2C + 0x14U))
#define I2C_TFR_CMD_FIFO_LVL (*(volatile uint32_t *)(BASE_I2C + 0x18U))

/* Bits del Registro TFR_CMD (Comando de Transferencia) */
#define I2C_TFR_CMD_STA      (1U << 9)  /* Generar START condition */
#define I2C_TFR_CMD_STO      (1U << 10) /* Generar STOP condition */
#define I2C_TFR_CMD_RW_D     (1U << 8)  /* 0 = Write, 1 = Read */

/* Bits del Registro CTRL (Control) */
#define I2C_CTRL_EN          (1U << 0)  /* Core Enable */

/* Bits del Registro STATUS (Estado) */
#define I2C_STATUS_CORE_BUSY (1U << 0)  /* 1 = I2C está transmitiendo */
#define I2C_STATUS_ARB_LOST  (1U << 1)  /* Arbitration Lost */
#define I2C_STATUS_NACK_DET  (1U << 2)  /* NACK Detectado */

/* =========================================================================
 * PROTOTIPOS DEL DRIVER HAL I2C (Arquitectura Non-Blocking via FIFO)
 * ========================================================================= */
void I2C_Init(void);
bool I2C_Is_Busy(void);
bool I2C_Write_FIFO(uint8_t dev_addr, const uint8_t *data, uint8_t len);
bool I2C_Read_Request(uint8_t dev_addr, uint8_t len);
bool I2C_Read_Rx_FIFO(uint8_t *buffer, uint8_t len);

#endif /* I2C_HAL_H */
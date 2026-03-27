#include "i2c_hal.h"
#include <stddef.h>

void I2C_Init(void) {
    I2C_CTRL_REG = I2C_CTRL_EN;
}

bool I2C_Is_Busy(void) {
    return ((I2C_STATUS_REG & I2C_STATUS_CORE_BUSY) != 0U);
}

bool I2C_Write_FIFO(uint8_t dev_addr, const uint8_t *data, uint8_t len) {
    if ((data == NULL) || (len == 0U) || (len > 30U)) return false;

    if ((I2C_STATUS_REG & I2C_STATUS_NACK_DET) != 0U) {
        I2C_STATUS_REG = I2C_STATUS_NACK_DET; 
    }

    uint32_t fifo_used = I2C_TFR_CMD_FIFO_LVL;
    if ((32U - fifo_used) < (uint32_t)(len + 1U)) return false; 

    I2C_TFR_CMD_REG = I2C_TFR_CMD_STA | (uint32_t)(dev_addr << 1);

    for (uint8_t i = 0U; i < len; i++) {
        uint32_t data_cmd = (uint32_t)data[i];
        if (i == (len - 1U)) data_cmd |= I2C_TFR_CMD_STO;
        I2C_TFR_CMD_REG = data_cmd;
    }
    return true;
}

bool I2C_Read_Request(uint8_t dev_addr, uint8_t len) {
    if (len == 0U || len > 30U) return false;
    
    I2C_TFR_CMD_REG = I2C_TFR_CMD_STA | (uint32_t)((dev_addr << 1) | 1U);
    
    for (uint8_t i = 0U; i < len; i++) {
        uint32_t cmd = I2C_TFR_CMD_RW_D; 
        if (i == (len - 1U)) cmd |= I2C_TFR_CMD_STO; 
        I2C_TFR_CMD_REG = cmd;
    }
    return true;
}

bool I2C_Read_Rx_FIFO(uint8_t *buffer, uint8_t len) {
    uint32_t rx_level = *(volatile uint32_t *)(I2C_BASE + 0x1CU);
    
    if (rx_level < (uint32_t)len) return false; 
    
    for (uint8_t i = 0U; i < len; i++) {
        buffer[i] = (uint8_t)(I2C_RX_DATA_REG & 0xFFU);
    }
    return true;
}
#include "i2c_hal.h"
#include <stddef.h>

/**
 * @brief Inicializa y habilita el núcleo I2C.
 */
void I2C_Init(void) {
    /* Habilitar el IP I2C de hardware. Las velocidades y prescalers 
     * ya fueron sintetizados en el Qsys a 100kHz. */
    I2C_CTRL_REG = I2C_CTRL_EN;
}

/**
 * @brief Evalúa si el hardware I2C está ocupado operando en el bus.
 * @return true si está ocupado, false si está inactivo.
 */
bool I2C_Is_Busy(void) {
    return ((I2C_STATUS_REG & I2C_STATUS_CORE_BUSY) != 0U);
}

/**
 * @brief Inyecta una trama completa en la FIFO de hardware I2C. (Non-blocking)
 * * @param dev_addr Dirección del esclavo (7-bits, justificada a la derecha).
 * @param data Puntero al arreglo constante de bytes a transmitir.
 * @param len Número de bytes a transmitir (Debe ser menor a 31).
 * @return true si la trama se inyectó en la FIFO, false si hubo error.
 */
bool I2C_Write_FIFO(uint8_t dev_addr, const uint8_t *data, uint8_t len) {
    if ((data == NULL) || (len == 0U) || (len > 30U)) {
        return false; /* Violación de restricciones (CERT C) */
    }

    /* 1. Verificar si hay NACK previo en el bus y limpiarlo si es necesario */
    if ((I2C_STATUS_REG & I2C_STATUS_NACK_DET) != 0U) {
        I2C_STATUS_REG = I2C_STATUS_NACK_DET; /* Escribir 1 para limpiar W1C */
    }

    /* 2. Evaluar si hay espacio suficiente en la FIFO de comandos (TFR_CMD)
     * Necesitamos: 1 byte para (ADDR + START) + 'len' bytes para datos */
    uint32_t fifo_used = I2C_TFR_CMD_FIFO_LVL;
    if ((32U - fifo_used) < (uint32_t)(len + 1U)) {
        return false; /* FIFO sin espacio, intentar en el siguiente ciclo */
    }

    /* 3. Inyectar START Condition + Dirección del dispositivo + Bit Write (0) */
    uint32_t addr_cmd = I2C_TFR_CMD_STA | (uint32_t)(dev_addr << 1);
    I2C_TFR_CMD_REG = addr_cmd;

    /* 4. Inyectar datos en la FIFO (El hardware los despachará automáticamente) */
    for (uint8_t i = 0U; i < len; i++) {
        uint32_t data_cmd = (uint32_t)data[i];
        
        /* Si es el último byte de la trama, adjuntar el bit de STOP condition */
        if (i == (len - 1U)) {
            data_cmd |= I2C_TFR_CMD_STO;
        }
        
        I2C_TFR_CMD_REG = data_cmd;
    }

    return true; /* Trama delegada con éxito al hardware */
}

/**
 * @brief Inyecta comandos de lectura en la FIFO. (Non-blocking)
 */
bool I2C_Read_Request(uint8_t dev_addr, uint8_t len) {
    if (len == 0U || len > 30U) return false;
    
    /* 1. START + Dirección con bit de Read (1) */
    I2C_TFR_CMD_REG = I2C_TFR_CMD_STA | (uint32_t)((dev_addr << 1) | 1U);
    
    /* 2. Emitir 'len' comandos de lectura al hardware */
    for (uint8_t i = 0U; i < len; i++) {
        uint32_t cmd = I2C_TFR_CMD_RW_D; /* Bit 8 en Alto indica LECTURA */
        if (i == (len - 1U)) {
            cmd |= I2C_TFR_CMD_STO; /* STOP en el último byte */
        }
        I2C_TFR_CMD_REG = cmd;
    }
    return true;
}

/**
 * @brief Extrae datos de la FIFO de recepción si están listos.
 */
bool I2C_Read_Rx_FIFO(uint8_t *buffer, uint8_t len) {
    /* Registro RX_FLR (0x1C) indica nivel de la FIFO de recepción */
    uint32_t rx_level = *(volatile uint32_t *)(BASE_I2C + 0x1CU);
    
    if (rx_level < (uint32_t)len) {
        return false; /* Aún no llegan todos los bytes */
    }
    
    for (uint8_t i = 0U; i < len; i++) {
        buffer[i] = (uint8_t)(I2C_RX_DATA_REG & 0xFFU);
    }
    return true;
}
#include "lcd_i2c.h"
#include "i2c_hal.h"
#include <stddef.h>

/* Mapeo de pines del PCF8574 a la LCD */
#define LCD_EN_BIT 0x04U
#define LCD_RW_BIT 0x02U
#define LCD_RS_BIT 0x01U
#define LCD_BL_BIT 0x08U /* Backlight ON */

typedef enum {
    STATE_LCD_BOOT = 0,
    STATE_LCD_INIT_W1,
    STATE_LCD_INIT_W2,
    STATE_LCD_INIT_W3,
    STATE_LCD_INIT_4BIT,
    STATE_LCD_CONFIG,
    STATE_LCD_IDLE,
    STATE_LCD_PRINTING
} LcdState_t;

/* Variables de estado estáticas encapsuladas */
static LcdState_t lcd_state = STATE_LCD_BOOT;
static uint64_t lcd_timer_ms = 0U;
static uint8_t config_step = 0U;

/* Buffer de transmisión asíncrona */
static char tx_buffer[LCD_COLUMNS + 1U];
static uint8_t tx_index = 0U;

/* Secuencia de comandos de configuración (HD44780) */
static const uint8_t init_cmds[] = {
    0x28U, /* Function set: 4-bit, 2 lines, 5x8 dots */
    0x0CU, /* Display ON, Cursor OFF, Blink OFF */
    0x06U, /* Entry mode: Increment, no shift */
    0x01U  /* Clear display */
};

/**
 * @brief Empaqueta 1 byte (Comando/Dato) en 4 bytes físicos para el PCF8574
 * generando los pulsos del pin EN (Enable Latch).
 */
static void Pack_I2C_Nibbles(uint8_t data, uint8_t mode_rs, uint8_t *i2c_buf) {
    uint8_t high_nibble = (data & 0xF0U) | LCD_BL_BIT | mode_rs;
    uint8_t low_nibble  = ((data << 4) & 0xF0U) | LCD_BL_BIT | mode_rs;
    
    i2c_buf[0] = high_nibble | LCD_EN_BIT; /* EN = 1 */
    i2c_buf[1] = high_nibble;              /* EN = 0 (Latch High) */
    i2c_buf[2] = low_nibble | LCD_EN_BIT;  /* EN = 1 */
    i2c_buf[3] = low_nibble;               /* EN = 0 (Latch Low) */
}

/**
 * @brief Inicializa la FSM. Llamar una vez en main().
 */
void LCD_Init_Async(void) {
    lcd_state = STATE_LCD_BOOT;
    lcd_timer_ms = 0U;
}

bool LCD_Is_Ready(void) {
    return (lcd_state == STATE_LCD_IDLE);
}

/**
 * @brief Despacha la cadena a la cola de transmisión.
 */
bool LCD_Print_String_Async(uint8_t row, uint8_t col, const char *str) {
    if (lcd_state != STATE_LCD_IDLE || str == NULL || col >= LCD_COLUMNS || row >= LCD_ROWS) {
        return false;
    }
    
    /* Configurar el cursor (DDRAM Address) */
    uint8_t row_offsets[] = {0x00U, 0x40U};
    uint8_t cmd = 0x80U | (col + row_offsets[row]);
    
    uint8_t i2c_cmd_buf[4];
    Pack_I2C_Nibbles(cmd, 0x00U, i2c_cmd_buf);
    
    /* Nota: Asumimos que el bus está libre. Forzamos la escritura de dirección. */
    (void)I2C_Write_FIFO(LCD_I2C_ADDR, i2c_cmd_buf, 4U);

    /* Copiar cadena al buffer estático (Evitando desbordamientos) */
    uint8_t i;
    for (i = 0U; i < LCD_COLUMNS && str[i] != '\0'; i++) {
        tx_buffer[i] = str[i];
    }
    tx_buffer[i] = '\0';
    tx_index = 0U;
    
    lcd_state = STATE_LCD_PRINTING;
    return true;
}

/**
 * @brief Máquina de estados principal. Llamar en CADA ciclo del Super-Loop.
 */
void LCD_Task_FSM(uint64_t current_time_ms) {
    uint8_t i2c_buf[4];
    
    switch (lcd_state) {
        case STATE_LCD_BOOT:
            if (current_time_ms > 50U) { /* Wait > 40ms */
                lcd_timer_ms = current_time_ms;
                lcd_state = STATE_LCD_INIT_W1;
            }
            break;
            
        case STATE_LCD_INIT_W1:
            if (!I2C_Is_Busy() && (current_time_ms - lcd_timer_ms) >= 5U) {
                Pack_I2C_Nibbles(0x30U, 0x00U, i2c_buf);
                (void)I2C_Write_FIFO(LCD_I2C_ADDR, i2c_buf, 4U);
                lcd_timer_ms = current_time_ms;
                lcd_state = STATE_LCD_INIT_W2;
            }
            break;
            
        case STATE_LCD_INIT_W2:
            if (!I2C_Is_Busy() && (current_time_ms - lcd_timer_ms) >= 1U) {
                Pack_I2C_Nibbles(0x30U, 0x00U, i2c_buf);
                (void)I2C_Write_FIFO(LCD_I2C_ADDR, i2c_buf, 4U);
                lcd_timer_ms = current_time_ms;
                lcd_state = STATE_LCD_INIT_W3;
            }
            break;
            
        /* ... Transición W3 y 4BIT análogas (omitidas por concisión, siguen el mismo patrón) ... */
        case STATE_LCD_INIT_W3:
            if (!I2C_Is_Busy() && (current_time_ms - lcd_timer_ms) >= 1U) {
                Pack_I2C_Nibbles(0x20U, 0x00U, i2c_buf); /* Enable 4-bit mode */
                (void)I2C_Write_FIFO(LCD_I2C_ADDR, i2c_buf, 4U);
                config_step = 0U;
                lcd_state = STATE_LCD_CONFIG;
            }
            break;
            
        case STATE_LCD_CONFIG:
            if (!I2C_Is_Busy()) {
                if (config_step < sizeof(init_cmds)) {
                    Pack_I2C_Nibbles(init_cmds[config_step], 0x00U, i2c_buf);
                    (void)I2C_Write_FIFO(LCD_I2C_ADDR, i2c_buf, 4U);
                    config_step++;
                } else {
                    lcd_state = STATE_LCD_IDLE;
                }
            }
            break;
            
        case STATE_LCD_PRINTING:
            if (!I2C_Is_Busy()) {
                if (tx_buffer[tx_index] != '\0') {
                    Pack_I2C_Nibbles((uint8_t)tx_buffer[tx_index], LCD_RS_BIT, i2c_buf);
                    (void)I2C_Write_FIFO(LCD_I2C_ADDR, i2c_buf, 4U);
                    tx_index++;
                } else {
                    lcd_state = STATE_LCD_IDLE; /* Transmisión finalizada */
                }
            }
            break;
            
        case STATE_LCD_IDLE:
        default:
            /* Esperando comandos externos. Sistema estable. */
            break;
    }
}

/**
 * @brief Formatea el reloj sin usar malloc ni librerías pesadas.
 * Salida garantizada de 12 bytes: "mm:SS:MMM:HH" + '\0'
 */
void LCD_Format_Time_String(uint8_t min, uint8_t sec, uint16_t millis, uint8_t hrs, char *out_buffer) {
    if (out_buffer == NULL) return;
    
    /* Decenas y unidades de Minutos */
    out_buffer[0] = (char)('0' + (min / 10U));
    out_buffer[1] = (char)('0' + (min % 10U));
    out_buffer[2] = ':';
    
    /* Decenas y unidades de Segundos */
    out_buffer[3] = (char)('0' + (sec / 10U));
    out_buffer[4] = (char)('0' + (sec % 10U));
    out_buffer[5] = ':';
    
    /* Centenas, Decenas y unidades de Milisegundos */
    out_buffer[6] = (char)('0' + (millis / 100U));
    out_buffer[7] = (char)('0' + ((millis / 10U) % 10U));
    out_buffer[8] = (char)('0' + (millis % 10U));
    out_buffer[9] = ':';
    
    /* Decenas y unidades de Horas */
    out_buffer[10] = (char)('0' + (hrs / 10U));
    out_buffer[11] = (char)('0' + (hrs % 10U));
    out_buffer[12] = '\0';
}
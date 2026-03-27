#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>
#include <stdbool.h>

/* Configuración de Hardware del PCF8574 */
#define LCD_I2C_ADDR        (0x27U) /* Puede ser 0x3FU dependiendo del chip */
#define LCD_COLUMNS         (16U)
#define LCD_ROWS            (2U)

/* Prototipos de la API No Bloqueante */
void LCD_Init_Async(void);
void LCD_Task_FSM(uint64_t current_time_ms);
bool LCD_Is_Ready(void);
bool LCD_Print_String_Async(uint8_t row, uint8_t col, const char *str);

/* Formateador de tiempo sin printf (mm:SS:MM:HH) */
void LCD_Format_Time_String(uint8_t min, uint8_t sec, uint16_t millis, uint8_t hrs, char *out_buffer);

#endif /* LCD_I2C_H */
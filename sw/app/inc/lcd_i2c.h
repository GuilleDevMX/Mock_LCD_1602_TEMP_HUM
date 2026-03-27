#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_I2C_ADDR (0x27U) 
#define LCD_COLUMNS  (16U)
#define LCD_ROWS     (2U)

void LCD_Init_Async(void);
void LCD_Task_FSM(uint64_t current_time_ms);
bool LCD_Is_Ready(void);
bool LCD_Print_String_Async(uint8_t row, uint8_t col, const char *str);
void LCD_Format_Time_String(uint8_t min, uint8_t sec, uint16_t millis, uint8_t hrs, char *out_buffer);

#endif /* LCD_I2C_H */
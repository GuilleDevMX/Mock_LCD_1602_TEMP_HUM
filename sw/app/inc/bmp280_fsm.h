#ifndef BMP280_FSM_H
#define BMP280_FSM_H

#include <stdint.h>
#include <stdbool.h>

#define BMP280_I2C_ADDR (0x76U) 

typedef struct {
    uint16_t dig_T1; int16_t  dig_T2; int16_t  dig_T3;
    uint16_t dig_P1; int16_t  dig_P2; int16_t  dig_P3;
    int16_t  dig_P4; int16_t  dig_P5; int16_t  dig_P6;
    int16_t  dig_P7; int16_t  dig_P8; int16_t  dig_P9;
} Bmp280_Calib_t;

typedef struct {
    int32_t  temp_c_x100; 
    uint32_t press_pa;    
    bool     valid;
} BMP280_Data_t;

void BMP280_Init_Async(void);
void BMP280_Task_FSM(uint64_t current_time_ms);
bool BMP280_Trigger_Measurement(void);
BMP280_Data_t BMP280_Get_Data(void);

#endif /* BMP280_FSM_H */
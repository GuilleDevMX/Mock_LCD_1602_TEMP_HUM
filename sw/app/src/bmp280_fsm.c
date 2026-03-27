#include "bmp280_fsm.h"
#include "i2c_hal.h"

typedef enum {
    BMP_STATE_BOOT = 0,
    BMP_STATE_REQ_CALIB_ADDR,
    BMP_STATE_REQ_CALIB_DATA,
    BMP_STATE_READ_CALIB,
    BMP_STATE_CONFIG,
    BMP_STATE_IDLE,
    BMP_STATE_TRIGGER,
    BMP_STATE_WAIT_MEASURE,
    BMP_STATE_REQ_DATA_ADDR,
    BMP_STATE_REQ_DATA_READ,
    BMP_STATE_FETCH_DATA
} Bmp280State_t;

static Bmp280State_t bmp_state = BMP_STATE_BOOT;
static uint64_t bmp_timer_ms = 0U;

static Bmp280_Calib_t calib_data;
static BMP280_Data_t  sensor_data = {0, 0, false};
static int32_t t_fine = 0; /* Variable global de Bosch para la resolución de temperatura */

void BMP280_Init_Async(void) {
    bmp_state = BMP_STATE_BOOT;
    sensor_data.valid = false;
}

bool BMP280_Trigger_Measurement(void) {
    if (bmp_state == BMP_STATE_IDLE) {
        bmp_state = BMP_STATE_TRIGGER;
        return true;
    }
    return false;
}

BMP280_Data_t BMP280_Get_Data(void) {
    return sensor_data;
}

/* =========================================================================
 * ALGORITMOS DE COMPENSACIÓN BOSCH (ARITMÉTICA ENTERA - NO FLOATS)
 * ========================================================================= */
static int32_t Compensate_Temperature(int32_t adc_T) {
    int32_t var1, var2, T;
    
    var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * ((int32_t)calib_data.dig_T3)) >> 14;
    
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T; /* Retorna Temp x 100 */
}

static uint32_t Compensate_Pressure(int32_t adc_P) {
    int64_t var1, var2, p;
    
    var1 = ((int64_t)t_fine) - 128000LL;
    var2 = var1 * var1 * (int64_t)calib_data.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib_data.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib_data.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib_data.dig_P3) >> 8) + ((var1 * (int64_t)calib_data.dig_P2) << 12);
    var1 = (((((int64_t)1LL) << 47) + var1)) * ((int64_t)calib_data.dig_P1) >> 33;
    
    if (var1 == 0LL) {
        return 0U; /* Evitar división por cero */
    }
    
    p = 1048576LL - adc_P;
    p = (((p << 31) - var2) * 3125LL) / var1;
    var1 = (((int64_t)calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib_data.dig_P8) * p) >> 19;
    
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib_data.dig_P7) << 4);
    return (uint32_t)(p >> 8); /* Retorna Pascales enteros (Pa) */
}

/* =========================================================================
 * MÁQUINA DE ESTADOS PRINCIPAL
 * ========================================================================= */
void BMP280_Task_FSM(uint64_t current_time_ms) {
    uint8_t buf[24];
    
    switch (bmp_state) {
        case BMP_STATE_BOOT:
            if (current_time_ms > 2U) { /* Boot time > 2ms */
                bmp_state = BMP_STATE_REQ_CALIB_ADDR;
            }
            break;
            
        case BMP_STATE_REQ_CALIB_ADDR:
            if (!I2C_Is_Busy()) {
                buf[0] = 0x88U; /* Registro inicio de calibración */
                (void)I2C_Write_FIFO(BMP280_I2C_ADDR, buf, 1U);
                bmp_state = BMP_STATE_REQ_CALIB_DATA;
            }
            break;
            
        case BMP_STATE_REQ_CALIB_DATA:
            if (!I2C_Is_Busy()) {
                (void)I2C_Read_Request(BMP280_I2C_ADDR, 24U);
                bmp_state = BMP_STATE_READ_CALIB;
            }
            break;
            
        case BMP_STATE_READ_CALIB:
            if (I2C_Read_Rx_FIFO(buf, 24U)) {
                /* Ensamblaje Little-Endian según datasheet */
                calib_data.dig_T1 = (uint16_t)(buf[0] | (buf[1] << 8));
                calib_data.dig_T2 = (int16_t)(buf[2] | (buf[3] << 8));
                calib_data.dig_T3 = (int16_t)(buf[4] | (buf[5] << 8));
                calib_data.dig_P1 = (uint16_t)(buf[6] | (buf[7] << 8));
                calib_data.dig_P2 = (int16_t)(buf[8] | (buf[9] << 8));
                calib_data.dig_P3 = (int16_t)(buf[10] | (buf[11] << 8));
                calib_data.dig_P4 = (int16_t)(buf[12] | (buf[13] << 8));
                calib_data.dig_P5 = (int16_t)(buf[14] | (buf[15] << 8));
                calib_data.dig_P6 = (int16_t)(buf[16] | (buf[17] << 8));
                calib_data.dig_P7 = (int16_t)(buf[18] | (buf[19] << 8));
                calib_data.dig_P8 = (int16_t)(buf[20] | (buf[21] << 8));
                calib_data.dig_P9 = (int16_t)(buf[22] | (buf[23] << 8));
                bmp_state = BMP_STATE_CONFIG;
            }
            break;
            
        case BMP_STATE_CONFIG:
            if (!I2C_Is_Busy()) {
                /* Config: Filter OFF, Standby 0.5ms */
                buf[0] = 0xF5U; buf[1] = 0x00U;
                (void)I2C_Write_FIFO(BMP280_I2C_ADDR, buf, 2U);
                bmp_state = BMP_STATE_IDLE;
            }
            break;
            
        case BMP_STATE_IDLE:
            /* Esperando orden de disparo */
            break;
            
        case BMP_STATE_TRIGGER:
            if (!I2C_Is_Busy()) {
                /* Ctrl_Meas: Osrs_T(x1), Osrs_P(x1), Mode(Forced) -> 0x25 */
                buf[0] = 0xF4U; buf[1] = 0x25U;
                (void)I2C_Write_FIFO(BMP280_I2C_ADDR, buf, 2U);
                bmp_timer_ms = current_time_ms;
                bmp_state = BMP_STATE_WAIT_MEASURE;
            }
            break;
            
        case BMP_STATE_WAIT_MEASURE:
            /* Oversampling x1 toma max 11.2ms según datasheet */
            if ((current_time_ms - bmp_timer_ms) > 12U) {
                bmp_state = BMP_STATE_REQ_DATA_ADDR;
            }
            break;
            
        case BMP_STATE_REQ_DATA_ADDR:
            if (!I2C_Is_Busy()) {
                buf[0] = 0xF7U; /* Registro base de presión */
                (void)I2C_Write_FIFO(BMP280_I2C_ADDR, buf, 1U);
                bmp_state = BMP_STATE_REQ_DATA_READ;
            }
            break;
            
        case BMP_STATE_REQ_DATA_READ:
            if (!I2C_Is_Busy()) {
                (void)I2C_Read_Request(BMP280_I2C_ADDR, 6U);
                bmp_state = BMP_STATE_FETCH_DATA;
            }
            break;
            
        case BMP_STATE_FETCH_DATA:
            if (I2C_Read_Rx_FIFO(buf, 6U)) {
                int32_t raw_P = (int32_t)((((uint32_t)buf[0]) << 12) | (((uint32_t)buf[1]) << 4) | (((uint32_t)buf[2]) >> 4));
                int32_t raw_T = (int32_t)((((uint32_t)buf[3]) << 12) | (((uint32_t)buf[4]) << 4) | (((uint32_t)buf[5]) >> 4));
                
                sensor_data.temp_c_x100 = Compensate_Temperature(raw_T);
                sensor_data.press_pa    = Compensate_Pressure(raw_P);
                sensor_data.valid       = true;
                
                bmp_state = BMP_STATE_IDLE;
            }
            break;
            
        default:
            bmp_state = BMP_STATE_BOOT;
            break;
    }
}
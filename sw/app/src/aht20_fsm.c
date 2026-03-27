#include "aht20_fsm.h"
#include "i2c_hal.h"

typedef enum {
    AHT_STATE_BOOT = 0,
    AHT_STATE_CHECK_CAL_CMD,
    AHT_STATE_EVAL_CAL_READ,
    AHT_STATE_SEND_CAL,
    AHT_STATE_IDLE,
    AHT_STATE_TRIGGER,
    AHT_STATE_WAIT_MEASURE,
    AHT_STATE_FETCH_DATA
} Aht20State_t;

static Aht20State_t aht_state = AHT_STATE_BOOT;
static uint64_t aht_timer_ms = 0U;
static AHT20_Data_t sensor_data = {0, 0, false};

void AHT20_Init_Async(void) {
    aht_state = AHT_STATE_BOOT;
    aht_timer_ms = 0U;
    sensor_data.valid = false;
}

bool AHT20_Trigger_Measurement(void) {
    if (aht_state == AHT_STATE_IDLE) {
        aht_state = AHT_STATE_TRIGGER;
        return true;
    }
    return false;
}

AHT20_Data_t AHT20_Get_Data(void) {
    return sensor_data;
}

void AHT20_Task_FSM(uint64_t current_time_ms) {
    uint8_t cmd_buf[3];
    uint8_t rx_buf[6];
    
    switch (aht_state) {
        case AHT_STATE_BOOT:
            if (current_time_ms > 40U) { /* Power-on delay */
                aht_state = AHT_STATE_CHECK_CAL_CMD;
            }
            break;
            
        case AHT_STATE_CHECK_CAL_CMD:
            if (!I2C_Is_Busy()) {
                (void)I2C_Read_Request(AHT20_I2C_ADDR, 1U);
                aht_state = AHT_STATE_EVAL_CAL_READ;
            }
            break;
            
        case AHT_STATE_EVAL_CAL_READ:
            if (I2C_Read_Rx_FIFO(rx_buf, 1U)) {
                if ((rx_buf[0] & 0x08U) != 0U) { /* Bit 3: Calibrated */
                    aht_state = AHT_STATE_IDLE;
                } else {
                    aht_state = AHT_STATE_SEND_CAL;
                }
            }
            break;
            
        case AHT_STATE_SEND_CAL:
            if (!I2C_Is_Busy()) {
                cmd_buf[0] = 0xBEU; cmd_buf[1] = 0x08U; cmd_buf[2] = 0x00U;
                (void)I2C_Write_FIFO(AHT20_I2C_ADDR, cmd_buf, 3U);
                aht_timer_ms = current_time_ms;
                aht_state = AHT_STATE_WAIT_MEASURE; /* Usamos el delay de 80ms por seguridad */
            }
            break;
            
        case AHT_STATE_IDLE:
            /* Esperando orden superior desde main() */
            break;
            
        case AHT_STATE_TRIGGER:
            if (!I2C_Is_Busy()) {
                cmd_buf[0] = 0xACU; cmd_buf[1] = 0x33U; cmd_buf[2] = 0x00U;
                (void)I2C_Write_FIFO(AHT20_I2C_ADDR, cmd_buf, 3U);
                aht_timer_ms = current_time_ms;
                aht_state = AHT_STATE_WAIT_MEASURE;
            }
            break;
            
        case AHT_STATE_WAIT_MEASURE:
            if ((current_time_ms - aht_timer_ms) >= 80U) { /* Wait 80ms */
                (void)I2C_Read_Request(AHT20_I2C_ADDR, 6U);
                aht_state = AHT_STATE_FETCH_DATA;
            }
            break;
            
        case AHT_STATE_FETCH_DATA:
            if (I2C_Read_Rx_FIFO(rx_buf, 6U)) {
                if ((rx_buf[0] & 0x80U) == 0U) { /* Bit 7: 0 = Busy ended */
                    /* Reensamblado de 20-bits según datasheet */
                    uint32_t raw_hum = ((uint32_t)rx_buf[1] << 12) | 
                                       ((uint32_t)rx_buf[2] << 4) | 
                                       ((uint32_t)rx_buf[3] >> 4);
                                       
                    uint32_t raw_tmp = (((uint32_t)rx_buf[3] & 0x0FU) << 16) | 
                                       ((uint32_t)rx_buf[4] << 8) | 
                                       (uint32_t)rx_buf[5];
                    
                    /* Matemáticas Fixed-Point optimizadas para RISC-V de 32 bits */
                    sensor_data.hum_rh_x10 = (raw_hum * 1000U) >> 20U;
                    sensor_data.temp_c_x10 = (int32_t)((raw_tmp * 2000U) >> 20U) - 500;
                    sensor_data.valid = true;
                }
                aht_state = AHT_STATE_IDLE;
            }
            break;
            
        default:
            aht_state = AHT_STATE_BOOT; /* Fallback de seguridad CERT C */
            break;
    }
}
#ifndef AHT20_FSM_H
#define AHT20_FSM_H

#include <stdint.h>
#include <stdbool.h>

#define AHT20_I2C_ADDR (0x38U)

typedef struct {
    int32_t  temp_c_x10;  
    uint32_t hum_rh_x10;  
    bool     valid;       
} AHT20_Data_t;

void AHT20_Init_Async(void);
void AHT20_Task_FSM(uint64_t current_time_ms);
bool AHT20_Trigger_Measurement(void);
AHT20_Data_t AHT20_Get_Data(void);

#endif /* AHT20_FSM_H */
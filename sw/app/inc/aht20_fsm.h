#ifndef AHT20_FSM_H
#define AHT20_FSM_H

#include <stdint.h>
#include <stdbool.h>

#define AHT20_I2C_ADDR (0x38U)

/* Estructura de datos sanitizada (Punto Fijo) */
typedef struct {
    int32_t  temp_c_x10;  /* Temperatura x 10 (ej. 253 = 25.3 C) */
    uint32_t hum_rh_x10;  /* Humedad x 10 (ej. 456 = 45.6 %) */
    bool     valid;       /* true si el CRC/Status es correcto */
} AHT20_Data_t;

void AHT20_Init_Async(void);
void AHT20_Task_FSM(uint64_t current_time_ms);
bool AHT20_Trigger_Measurement(void);
AHT20_Data_t AHT20_Get_Data(void);

#endif /* AHT20_FSM_H */
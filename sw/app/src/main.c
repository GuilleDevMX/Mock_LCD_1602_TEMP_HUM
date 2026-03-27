#include "system_hardware.h"
#include "i2c_hal.h"
#include "lcd_i2c.h"
#include "aht20_fsm.h"
#include "bmp280_fsm.h"

/* =========================================================================
 * VARIABLES DE TIEMPO CASCADA (Evita costosas operaciones de módulo/división)
 * ========================================================================= */
static uint64_t sys_uptime_ms = 0U;
static uint16_t time_ms = 0U;
static uint8_t  time_sec = 0U;
static uint8_t  time_min = 0U;
static uint8_t  time_hr = 0U;

/* =========================================================================
 * ESTADOS DEL ORQUESTADOR
 * ========================================================================= */
typedef enum {
    APP_STATE_INIT_HW = 0,
    APP_STATE_WAIT_INIT,
    APP_STATE_RUNNING
} AppState_t;

/* =========================================================================
 * FUNCIONES DE FORMATEO LIGERO (Cero footprint de librería estándar)
 * ========================================================================= */
static void Format_Temp_Hum(char *buf, int32_t t_x100, uint32_t h_x10) {
    /* Formato: "T:XX.XC H:XX.X%" (Ajustado a 16 chars) */
    int32_t temp = t_x100 / 10; /* Reducir a un decimal para ahorrar espacio */
    if (temp < 0) { buf[2] = '-'; temp = -temp; } else { buf[2] = ' '; }
    
    buf[0] = 'T'; buf[1] = ':';
    buf[3] = (char)('0' + ((temp / 100) % 10));
    buf[4] = (char)('0' + ((temp / 10) % 10));
    buf[5] = '.';
    buf[6] = (char)('0' + (temp % 10));
    buf[7] = 'C'; buf[8] = ' '; buf[9] = 'H'; buf[10] = ':';
    
    buf[11] = (char)('0' + ((h_x10 / 100) % 10));
    buf[12] = (char)('0' + ((h_x10 / 10) % 10));
    buf[13] = '.';
    buf[14] = (char)('0' + (h_x10 % 10));
    buf[15] = '%'; buf[16] = '\0';
}

static void Format_Pressure(char *buf, uint32_t p_pa) {
    /* Formato: "P: XXXXXX Pa    " */
    buf[0] = 'P'; buf[1] = ':'; buf[2] = ' ';
    buf[3] = (char)('0' + ((p_pa / 100000U) % 10U));
    buf[4] = (char)('0' + ((p_pa / 10000U) % 10U));
    buf[5] = (char)('0' + ((p_pa / 1000U) % 10U));
    buf[6] = (char)('0' + ((p_pa / 100U) % 10U));
    buf[7] = (char)('0' + ((p_pa / 10U) % 10U));
    buf[8] = (char)('0' + (p_pa % 10U));
    buf[9] = ' '; buf[10] = 'P'; buf[11] = 'a'; 
    buf[12] = ' '; buf[13] = ' '; buf[14] = ' '; buf[15] = ' '; buf[16] = '\0';
}

/* =========================================================================
 * ACTUALIZACIÓN DEL RELOJ DE HARDWARE
 * ========================================================================= */
static void System_Update_Tick(void) {
    if ((TIMER_STATUS_REG & TIMER_STATUS_TO_BIT) != 0U) {
        TIMER_STATUS_REG = 0x01U; /* Limpiar bandera */
        sys_uptime_ms++;
        
        time_ms++;
        if (time_ms >= 1000U) {
            time_ms = 0U;
            time_sec++;
            if (time_sec >= 60U) {
                time_sec = 0U;
                time_min++;
                if (time_min >= 60U) {
                    time_min = 0U;
                    time_hr++;
                }
            }
        }
    }
}

/* =========================================================================
 * ENTRY POINT PRINCIPAL
 * ========================================================================= */
int main(void) {
    AppState_t app_state = APP_STATE_INIT_HW;
    
    uint64_t last_sensor_trigger = 0U;
    uint64_t last_lcd_update = 0U;
    bool show_pressure_screen = true;
    
    char str_row0[17];
    char str_row1[17];
    
    /* Configuración Inicial de Bajo Nivel */
    TIMER_STATUS_REG = 0x01U; /* Limpiar timer obsoleto */
    I2C_Init();
    UART_Tx_String("\r\n[SYS] Nios V/c Deterministic IIoT Booting...\r\n");

    /* Super-Loop Infinito */
    for (;;) {
        /* 1. Orquestación del Tiempo (Jitter-free) */
        System_Update_Tick();
        
        /* 2. Ejecución de las Máquinas de Estado Esclavas */
        LCD_Task_FSM(sys_uptime_ms);
        AHT20_Task_FSM(sys_uptime_ms);
        BMP280_Task_FSM(sys_uptime_ms);
        
        /* 3. Máquina de Estado Maestra (Lógica de Aplicación) */
        switch (app_state) {
            
            case APP_STATE_INIT_HW:
                LCD_Init_Async();
                AHT20_Init_Async();
                BMP280_Init_Async();
                app_state = APP_STATE_WAIT_INIT;
                break;
                
            case APP_STATE_WAIT_INIT:
                /* Esperar a que la LCD despierte (toma ~50ms) */
                if (LCD_Is_Ready()) {
                    UART_Tx_String("[SYS] FSMs Inicializadas.\r\n");
                    app_state = APP_STATE_RUNNING;
                }
                break;
                
            case APP_STATE_RUNNING:
                /* A. Disparador de Sensores (Cada 2000 ms) */
                if ((sys_uptime_ms - last_sensor_trigger) >= 2000U) {
                    (void)AHT20_Trigger_Measurement();
                    (void)BMP280_Trigger_Measurement();
                    show_pressure_screen = !show_pressure_screen; /* Alternar UI */
                    last_sensor_trigger = sys_uptime_ms;
                }
                
                /* B. Disparador de Pantalla LCD (Cada 150 ms para fluidez) */
                if (LCD_Is_Ready() && ((sys_uptime_ms - last_lcd_update) >= 150U)) {
                    BMP280_Data_t bmp_data = BMP280_Get_Data();
                    AHT20_Data_t  aht_data = AHT20_Get_Data();
                    
                    /* Formatear Fila 0: Temp (del BMP280, es más preciso) y Hum (del AHT20) */
                    if (bmp_data.valid && aht_data.valid) {
                        Format_Temp_Hum(str_row0, bmp_data.temp_c_x100, aht_data.hum_rh_x10);
                    } else {
                        Format_Temp_Hum(str_row0, 0, 0); /* Datos pendientes */
                    }
                    
                    /* Formatear Fila 1: Alterna entre Presión y Reloj */
                    if (show_pressure_screen) {
                        if (bmp_data.valid) {
                            Format_Pressure(str_row1, bmp_data.press_pa);
                        }
                    } else {
                        str_row1[0] = ' '; str_row1[1] = ' '; /* Padding */
                        LCD_Format_Time_String(time_min, time_sec, time_ms, time_hr, &str_row1[2]);
                        str_row1[14] = ' '; str_row1[15] = ' '; str_row1[16] = '\0';
                    }
                    
                    /* Despachar asíncronamente a la LCD */
                    (void)LCD_Print_String_Async(0, 0, str_row0);
                    (void)LCD_Print_String_Async(1, 0, str_row1);
                    
                    last_lcd_update = sys_uptime_ms;
                }
                break;
                
            default:
                app_state = APP_STATE_INIT_HW;
                break;
        }
    }
    
    return 0; /* Unreachable */
}
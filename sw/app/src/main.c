/**
 * @file    main.c
 * @brief   IIoT Edge Node - Monolito de Diagnóstico con UART
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "system.h"

/* ==========================================================================
 * [DOMINIO 1] MAPEO DE HARDWARE (HAL)
 * ========================================================================== */
#define TIMER_STATUS_REG    (*(volatile uint16_t *)(TIMER_BASE + 0x00U))
#define TIMER_STATUS_TO_BIT (0x01U)

#define I2C_TFR_CMD_REG      (*(volatile uint32_t *)(I2C_BASE + 0x00U))
#define I2C_RX_DATA_REG      (*(volatile uint32_t *)(I2C_BASE + 0x04U))
#define I2C_CTRL_REG         (*(volatile uint32_t *)(I2C_BASE + 0x08U))
#define I2C_STATUS_REG       (*(volatile uint32_t *)(I2C_BASE + 0x14U))
#define I2C_TFR_CMD_FIFO_LVL (*(volatile uint32_t *)(I2C_BASE + 0x18U))

#define I2C_TFR_CMD_STA      (1U << 9)
#define I2C_TFR_CMD_STO      (1U << 10)
#define I2C_TFR_CMD_RW_D     (1U << 8)
#define I2C_STATUS_CORE_BUSY (1U << 0)

/* UART PARA DIAGNÓSTICO */
#define UART_TXDATA_REG     (*(volatile uint16_t *)(UART_BASE + 0x04U))
#define UART_STATUS_REG     (*(volatile uint16_t *)(UART_BASE + 0x08U))
#define UART_STATUS_TRDY    (0x40U)

static void UART_Tx(const char *str) {
    if (str == NULL) return;
    while (*str != '\0') {
        while ((UART_STATUS_REG & UART_STATUS_TRDY) == 0U);
        UART_TXDATA_REG = (uint16_t)*str++;
    }
}

/* Variables Globales de Tiempo */
static uint64_t sys_uptime_ms = 0U;

/* ==========================================================================
 * [DOMINIO 2] DRIVER I2C
 * ========================================================================== */
static void I2C_Init(void) { I2C_CTRL_REG = 1U; }
static inline bool I2C_Is_Busy(void) { return ((I2C_STATUS_REG & I2C_STATUS_CORE_BUSY) != 0U); }

static bool I2C_Write_FIFO(uint8_t dev_addr, const uint8_t *data, uint8_t len) {
    if ((32U - I2C_TFR_CMD_FIFO_LVL) < (uint32_t)(len + 1U)) return false; 
    I2C_TFR_CMD_REG = I2C_TFR_CMD_STA | (uint32_t)(dev_addr << 1);
    for (uint8_t i = 0U; i < len; i++) {
        uint32_t data_cmd = (uint32_t)data[i];
        if (i == (len - 1U)) data_cmd |= I2C_TFR_CMD_STO;
        I2C_TFR_CMD_REG = data_cmd;
    }
    return true;
}

/* ==========================================================================
 * [DOMINIO 3] FSM MINIMALISTA LCD 16x02
 * ========================================================================== */
#define LCD_ADDR 0x27U
typedef enum { LCD_BOOT = 0, LCD_INIT_W1, LCD_INIT_W2, LCD_INIT_W3, LCD_CONFIG, LCD_IDLE } LcdState_t;

static LcdState_t lcd_state = LCD_BOOT;
static uint64_t lcd_timer = 0U;
static uint8_t lcd_step = 0U;
static const uint8_t lcd_init_cmds[] = {0x28U, 0x0CU, 0x06U, 0x01U};

static void Pack_Nibbles(uint8_t data, uint8_t rs, uint8_t *buf) {
    uint8_t hn = (data & 0xF0U) | 0x08U | rs;
    uint8_t ln = ((data << 4) & 0xF0U) | 0x08U | rs;
    buf[0] = hn | 0x04U; buf[1] = hn; buf[2] = ln | 0x04U; buf[3] = ln;
}

static void LCD_Task(void) {
    uint8_t buf[4];
    switch (lcd_state) {
        case LCD_BOOT: 
            if (sys_uptime_ms > 50U) { lcd_timer = sys_uptime_ms; lcd_state = LCD_INIT_W1; UART_Tx("[DIAG] Boot timeout OK. Iniciando I2C...\r\n"); } 
            break;
        case LCD_INIT_W1: 
            if (!I2C_Is_Busy() && (sys_uptime_ms - lcd_timer) >= 5U) { Pack_Nibbles(0x30U,0,buf); I2C_Write_FIFO(LCD_ADDR,buf,4); lcd_timer = sys_uptime_ms; lcd_state = LCD_INIT_W2; UART_Tx("[DIAG] W1 Enviado.\r\n");} 
            break;
        case LCD_INIT_W2: 
            if (!I2C_Is_Busy() && (sys_uptime_ms - lcd_timer) >= 1U) { Pack_Nibbles(0x30U,0,buf); I2C_Write_FIFO(LCD_ADDR,buf,4); lcd_timer = sys_uptime_ms; lcd_state = LCD_INIT_W3; UART_Tx("[DIAG] W2 Enviado.\r\n");} 
            break;
        case LCD_INIT_W3: 
            if (!I2C_Is_Busy() && (sys_uptime_ms - lcd_timer) >= 1U) { Pack_Nibbles(0x20U,0,buf); I2C_Write_FIFO(LCD_ADDR,buf,4); lcd_step = 0; lcd_state = LCD_CONFIG; } 
            break;
        case LCD_CONFIG:
            if (!I2C_Is_Busy()) {
                if (lcd_step < 4) { Pack_Nibbles(lcd_init_cmds[lcd_step++],0,buf); I2C_Write_FIFO(LCD_ADDR,buf,4); }
                else { lcd_state = LCD_IDLE; UART_Tx("[DIAG] LCD CONFIGURADA CON EXITO.\r\n"); }
            } 
            break;
        default: break;
    }
}

/* ==========================================================================
 * BUCLE PRINCIPAL
 * ========================================================================== */
int main(void) {
    TIMER_STATUS_REG = 0x01U; 
    I2C_Init();
    
    UART_Tx("\r\n================================\r\n");
    UART_Tx("[SYS] Firmware de Diagnostico V2\r\n");
    UART_Tx("================================\r\n");

    uint64_t last_tick_msg = 0U;

    for (;;) {
        /* Tick Hardware */
        if ((TIMER_STATUS_REG & TIMER_STATUS_TO_BIT) != 0U) {
            TIMER_STATUS_REG = 0x01U; 
            sys_uptime_ms++;
        }
        
        /* Ejecutar solo la LCD para aislar el problema */
        LCD_Task();
        
        /* Heartbeat en Serial cada 2 segundos para probar que la CPU no está congelada */
        if ((sys_uptime_ms - last_tick_msg) >= 2000U) {
            if (I2C_Is_Busy()) {
                UART_Tx("[ALERTA] Bus I2C atascado en BUSY.\r\n");
            } else {
                UART_Tx("[OK] Sistema corriendo. Bus I2C Libre.\r\n");
            }
            last_tick_msg = sys_uptime_ms;
        }
    }
    return 0;
}
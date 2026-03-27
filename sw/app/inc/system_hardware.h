#ifndef SYSTEM_HARDWARE_H
#define SYSTEM_HARDWARE_H

#include <stdint.h>
#include "system.h" /* Única Fuente de Verdad (BSP) */

/* =========================================================================
 * REGISTROS DEL TIMER DE ALTERA (Tick de 1ms)
 * ========================================================================= */
#define TIMER_STATUS_REG    (*(volatile uint16_t *)(TIMER_BASE + 0x00U))
#define TIMER_CONTROL_REG   (*(volatile uint16_t *)(TIMER_BASE + 0x04U))

#define TIMER_STATUS_TO_BIT (0x01U)

/* =========================================================================
 * REGISTROS DEL UART DE ALTERA (Depuración Ligera)
 * ========================================================================= */
#define UART_RXDATA_REG     (*(volatile uint16_t *)(UART_BASE + 0x00U))
#define UART_TXDATA_REG     (*(volatile uint16_t *)(UART_BASE + 0x04U))
#define UART_STATUS_REG     (*(volatile uint16_t *)(UART_BASE + 0x08U))
#define UART_CONTROL_REG    (*(volatile uint16_t *)(UART_BASE + 0x0CU))

#define UART_STATUS_TRDY    (0x40U) /* Transmit Ready */

/* =========================================================================
 * PROTOTIPOS DE DEPURACIÓN SEGURA
 * ========================================================================= */
void UART_Tx_Char(const char c);
void UART_Tx_String(const char *str);

#endif /* SYSTEM_HARDWARE_H */
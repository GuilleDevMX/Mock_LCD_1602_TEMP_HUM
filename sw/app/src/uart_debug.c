#include "system_hardware.h"
#include <stddef.h>

void UART_Tx_Char(const char c) {
    /* Polling bloqueante intencional solo para depuración */
    while ((UART_STATUS_REG & UART_STATUS_TRDY) == 0U) {
        /* Esperar a que el buffer TX se vacíe */
    }
    UART_TXDATA_REG = (uint16_t)c;
}

void UART_Tx_String(const char *str) {
    if (str == NULL) { return; }
    while (*str != '\0') {
        UART_Tx_Char(*str);
        str++;
    }
}
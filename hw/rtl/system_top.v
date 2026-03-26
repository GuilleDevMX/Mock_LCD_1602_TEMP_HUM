// ============================================================================
// Archivo: system_top.v
// Dominio: Hardware (RTL)
// Descripción: Top-level wrapper para el SoC Nios V/c.
//              Implementa la lógica Tri-state para el bus I2C.
// ============================================================================

module system_top (
    // Reloj y Reset del Sistema
    input  wire clk_50m,    // Reloj principal (Ej. 50 MHz del oscilador)
    input  wire rst_n,      // Botón de reset físico (Activo en bajo)

    // Bus I2C de Sensores y LCD (Puertos Bidireccionales)
    inout  wire i2c_sda,    // Serial Data Line
    inout  wire i2c_scl,    // Serial Clock Line

    // UART de Depuración (RS232)
    input  wire uart_rx,    // Recepción desde la PC
    output wire uart_tx     // Transmisión hacia la PC
);

    // ========================================================================
    // SEÑALES INTERNAS DEL BUS I2C
    // ========================================================================
    wire sda_in_w;
    wire scl_in_w;
    wire sda_oe_w;
    wire scl_oe_w;

    // ========================================================================
    // INFERENCIA DE BUFFERS TRI-ESTADO (OPEN-DRAIN)
    // ========================================================================
    // Regla de Diseño HW: Si el IP de Altera habilita la salida (oe == 1), 
    // forzamos la línea a 0 (GND). Si no (oe == 0), la dejamos en alta 
    // impedancia (1'bz) para que el pull-up externo suba el nivel a VCC.
    // ========================================================================
    assign i2c_sda = sda_oe_w ? 1'b0 : 1'bz;
    assign i2c_scl = scl_oe_w ? 1'b0 : 1'bz;

    // La lectura de la línea física se inyecta directamente al IP
    assign sda_in_w = i2c_sda;
    assign scl_in_w = i2c_scl;

    // ========================================================================
    // INSTANCIACIÓN DEL SUBSISTEMA SoC (Platform Designer)
    // ========================================================================
    system_soc u0 (
        .clk_clk           (clk_50m),   // Reloj del sistema
        .reset_reset_n     (rst_n),     // Reset del sistema

        // Conexiones I2C segregadas del IP
        .com_export_sda_in (sda_in_w), 
        .com_export_scl_in (scl_in_w), 
        .com_export_sda_oe (sda_oe_w), 
        .com_export_scl_oe (scl_oe_w), 

        // Conexiones UART
        .debug_export_rxd  (uart_rx),  
        .debug_export_txd  (uart_tx)   
    );

endmodule
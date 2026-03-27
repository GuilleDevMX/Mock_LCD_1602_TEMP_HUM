module system_top (
    input  wire clk_50m,
    input  wire rst_n,
    inout  wire i2c_sda,
    inout  wire i2c_scl,
    input  wire uart_rx,
    output wire uart_tx
);

    wire sda_oe_w;
    wire scl_oe_w;
    wire sda_in_w;
    wire scl_in_w;

    // ====================================================================
    // MAGIA DE HARDWARE: INFERENCIA DE TRI-STATE (OPEN-DRAIN)
    // ====================================================================
    // Si el IP quiere escribir un 0 (oe == 1), lo tiramos a GND (1'b0).
    // Si quiere escribir un 1 (oe == 0), lo dejamos flotar en Alta Impedancia (1'bz).
    assign i2c_sda = sda_oe_w ? 1'b0 : 1'bz;
    assign i2c_scl = scl_oe_w ? 1'b0 : 1'bz;

    assign sda_in_w = i2c_sda;
    assign scl_in_w = i2c_scl;

    system_soc u0 (
        .clk_clk           (clk_50m),
        .reset_reset_n     (rst_n),
        .com_export_sda_in (sda_in_w),
        .com_export_scl_in (scl_in_w),
        .com_export_sda_oe (sda_oe_w),
        .com_export_scl_oe (scl_oe_w),
        .debug_export_rxd  (uart_rx),
        .debug_export_txd  (uart_tx)
    );
endmodule
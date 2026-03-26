
module system_soc (
	clk_clk,
	reset_reset_n,
	com_export_sda_in,
	com_export_scl_in,
	com_export_sda_oe,
	com_export_scl_oe,
	debug_export_rxd,
	debug_export_txd);	

	input		clk_clk;
	input		reset_reset_n;
	input		com_export_sda_in;
	input		com_export_scl_in;
	output		com_export_sda_oe;
	output		com_export_scl_oe;
	input		debug_export_rxd;
	output		debug_export_txd;
endmodule

	system_soc u0 (
		.clk_clk           (<connected-to-clk_clk>),           //          clk.clk
		.reset_reset_n     (<connected-to-reset_reset_n>),     //        reset.reset_n
		.com_export_sda_in (<connected-to-com_export_sda_in>), //   com_export.sda_in
		.com_export_scl_in (<connected-to-com_export_scl_in>), //             .scl_in
		.com_export_sda_oe (<connected-to-com_export_sda_oe>), //             .sda_oe
		.com_export_scl_oe (<connected-to-com_export_scl_oe>), //             .scl_oe
		.debug_export_rxd  (<connected-to-debug_export_rxd>),  // debug_export.rxd
		.debug_export_txd  (<connected-to-debug_export_txd>)   //             .txd
	);


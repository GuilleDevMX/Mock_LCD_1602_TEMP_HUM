	component system_soc is
		port (
			clk_clk           : in  std_logic := 'X'; -- clk
			com_export_sda_in : in  std_logic := 'X'; -- sda_in
			com_export_scl_in : in  std_logic := 'X'; -- scl_in
			com_export_sda_oe : out std_logic;        -- sda_oe
			com_export_scl_oe : out std_logic;        -- scl_oe
			debug_export_rxd  : in  std_logic := 'X'; -- rxd
			debug_export_txd  : out std_logic;        -- txd
			reset_reset_n     : in  std_logic := 'X'  -- reset_n
		);
	end component system_soc;

	u0 : component system_soc
		port map (
			clk_clk           => CONNECTED_TO_clk_clk,           --          clk.clk
			com_export_sda_in => CONNECTED_TO_com_export_sda_in, --   com_export.sda_in
			com_export_scl_in => CONNECTED_TO_com_export_scl_in, --             .scl_in
			com_export_sda_oe => CONNECTED_TO_com_export_sda_oe, --             .sda_oe
			com_export_scl_oe => CONNECTED_TO_com_export_scl_oe, --             .scl_oe
			debug_export_rxd  => CONNECTED_TO_debug_export_rxd,  -- debug_export.rxd
			debug_export_txd  => CONNECTED_TO_debug_export_txd,  --             .txd
			reset_reset_n     => CONNECTED_TO_reset_reset_n      --        reset.reset_n
		);


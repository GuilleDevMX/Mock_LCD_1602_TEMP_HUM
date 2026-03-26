// (C) 2001-2025 Altera Corporation. All rights reserved.
// Your use of Altera Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files from any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Altera Program License Subscription 
// Agreement, Altera IP License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Altera and sold by 
// Altera or its authorized distributors.  Please refer to the applicable 
// agreement for further details.


module system_soc_CPU_core #(
    parameter int             MXLEN            = 32,
    parameter [MXLEN-1:0]     HARTID           = 'h0,
    parameter int             ALULEN           = MXLEN,
    parameter [MXLEN-1:0]     RESET_VECTOR     = 'h0,

    //Optional Debug
    parameter bit             DEBUG_ENA        = 1'b0,
    parameter [MXLEN -1:0]    DEBUG_VECTOR     = 'h8000_0000,
    parameter [MXLEN -1:0]    PARKLOOP_VECTOR  = DEBUG_VECTOR + 'd24,

    //Optional Zicsr extensions
    parameter bit             Zicsr_ENA        = 1'b0,
    parameter bit             Zicntr_ENA       = 1'b0,

    //Optional C-extensions
    parameter bit             RVC_ENA          = 1'b0,

    //Optional B-extensions
    parameter bit             Zba_ENA          = 1'b0,
    parameter bit             Zbb_ENA          = 1'b0,
    parameter bit             Zbc_ENA          = 1'b0,
    parameter bit             Zbs_ENA          = 1'b0,

   //Optional M-extensions
    parameter bit             Zmmul_ENA        = 1'b0,

    parameter bit             INSTR_REGISTERED = 1'b0,
    parameter bit             DATA_REGISTERED  = 1'b0,
    parameter bit             FAST_SHIFT       = 1'b0,
    parameter bit             ECC_ENA          = 1'b0,
    parameter bit             TARGET_AREA      = 1'b1,
    parameter                 DEVICE_FAMILY    = "Stratix 10"
)
(
    //Clock and reset(s)
    input  wire               clk,
    input  wire               reset,
    input  wire               reset_req,
    output wire               reset_req_ack,

    //AXI-4 Lite Instruction Interface
    //Write Address
    output wire [MXLEN  -1:0] instr_awaddr,
    output wire [        2:0] instr_awprot,
    output wire               instr_awvalid,
    input  wire               instr_awready,

    //Write Data
    output wire [MXLEN  -1:0] instr_wdata,
    output wire [MXLEN/8-1:0] instr_wstrb,
    output wire               instr_wvalid,
    input  wire               instr_wready,

    //Write Response
    input  wire [        1:0] instr_bresp,
    input  wire               instr_bvalid,
    output wire               instr_bready,

    //Read Address
    output wire [MXLEN  -1:0] instr_araddr,
    output wire [        2:0] instr_arprot,
    output wire               instr_arvalid,
    input  wire               instr_arready,

    //Read Data
    input  wire [MXLEN  -1:0] instr_rdata,
    input  wire [        1:0] instr_rresp,
    input  wire               instr_rvalid,
    output wire               instr_rready,

    //AXI-4 Lite Instruction Interface
    //Write Address
    output wire [MXLEN  -1:0] data_awaddr,
    output wire [        2:0] data_awprot,
    output wire               data_awvalid,
    input  wire               data_awready,

    //Write Data
    output wire [MXLEN  -1:0] data_wdata,
    output wire [MXLEN/8-1:0] data_wstrb,
    output wire               data_wvalid,
    input  wire               data_wready,

    //Write Response
    input  wire [        1:0] data_bresp,
    input  wire               data_bvalid,
    output wire               data_bready,

    //Read Address
    output wire [MXLEN  -1:0] data_araddr,
    output wire [        2:0] data_arprot,
    output wire               data_arvalid,
    input  wire               data_arready,

    //Read Data
    input  wire [MXLEN  -1:0] data_rdata,
    input  wire [        1:0] data_rresp,
    input  wire               data_rvalid,
    output wire               data_rready,

    //Interrupts
    input  wire               irq_debug,
    input  wire               irq_timer,
    input  wire               irq_ext,
    input  wire               irq_sw,
    input  wire [MXLEN -17:0] irq_plat_vec,

    //ECC
    output wire [        1:0] core_ecc_status,
    output wire [        3:0] core_ecc_src

`ifdef RISCV_FORMAL
    ,
    output wire               rvfi_valid,
    output wire [       63:0] rvfi_order,
    output wire [MXLEN  -1:0] rvfi_insn,
    output wire               rvfi_trap,
    output wire               rvfi_halt,
    output wire               rvfi_intr,
    output wire [        1:0] rvfi_mode,
    output wire [        1:0] rvfi_ixl,
    output wire [        4:0] rvfi_rs1_addr,
    output wire [        4:0] rvfi_rs2_addr,
    output wire [MXLEN  -1:0] rvfi_rs1_rdata,
    output wire [MXLEN  -1:0] rvfi_rs2_rdata,
    output wire [        4:0] rvfi_rd_addr,
    output wire [MXLEN  -1:0] rvfi_rd_wdata,
    output wire               rvfi_rd_wren,
    output wire [MXLEN  -1:0] rvfi_pc_rdata,
    output wire [MXLEN  -1:0] rvfi_pc_wdata,
    output wire [MXLEN  -1:0] rvfi_mem_addr,
    output wire [MXLEN  -1:0] rvfi_mem_rmask,
    output wire [MXLEN  -1:0] rvfi_mem_wmask,
    output wire [MXLEN  -1:0] rvfi_mem_rdata,
    output wire [MXLEN  -1:0] rvfi_mem_wdata
`endif
`ifdef NIOSV_DV
    ,
    input  wire [        1:0] rs1_ecc_force,
    output wire [        1:0] rs1_ecc_status,
    input  wire [        1:0] rs2_ecc_force,
    output wire [        1:0] rs2_ecc_status
`endif
);

    /**
     * Internal signals
     */
    wire               instruction;
    wire [MXLEN  -1:0] address;
    wire [MXLEN/8-1:0] byteenable;
    wire               read;
    wire [MXLEN  -1:0] readinstruction;
    wire [MXLEN  -1:0] readdata;
    wire               write;
    wire [MXLEN  -1:0] writedata;
    wire               done;

   //Keep RISCV-FORMAL signals if they're not exported
   //These are used by signaltap
`ifndef RISCV_FORMAL
  (* keep *) logic               rvfi_valid;             //asserted when the core retires an instruction
             logic [       63:0] rvfi_order;             //instruction index
  (* keep *) logic [MXLEN  -1:0] rvfi_insn;              //instruction word for the retired instruction
             logic               rvfi_trap;              //asserted when instruction can not be decoded (i.e. illegal)
                                                         //              misaligned memory read/write
                                                         //              jump to misaligned instruction address
             logic               rvfi_halt;              //asserted when the instruction is the last retired instruction before the core halts ???
             logic               rvfi_intr;              //asserted for the first instruction of a trap handler
             logic [        1:0] rvfi_mode;              //privilege mode
             logic [        1:0] rvfi_ixl;               //XLEN of current privilege mode (1==32, 2==64)
             logic [        4:0] rvfi_rs1_addr;          //decoded RS1 address of retired instruction
             logic [        4:0] rvfi_rs2_addr;          //decoded RS2 address of retired instruction
             logic [MXLEN  -1:0] rvfi_rs1_rdata;         //value of RS1 before execution of instruction
             logic [MXLEN  -1:0] rvfi_rs2_rdata;         //value of RS2 before execution of instruction
             logic [        4:0] rvfi_rd_addr;           //decoded RD address of retired instruction
             logic [MXLEN  -1:0] rvfi_rd_wdata;          //value of RD after execution of instruction
             logic               rvfi_rd_wren;           //write enable for RD (Altera extension)
  (* keep *) logic [MXLEN  -1:0] rvfi_pc_rdata;          //address of retired instruction
             logic [MXLEN  -1:0] rvfi_pc_wdata;          //address of next instruction
             logic [MXLEN  -1:0] rvfi_mem_addr;          //access memory address
             logic [MXLEN/8-1:0] rvfi_mem_rmask;         //bitmask that specifies which bytes in rdata are valid
             logic [MXLEN/8-1:0] rvfi_mem_wmask;         //bitmask that specifies which bytes in wdata are valid
             logic [MXLEN  -1:0] rvfi_mem_rdata;         //pre-state data read from rvfi_mem_addr
             logic [MXLEN  -1:0] rvfi_mem_wdata;         //post-state data written to rvfi_mem_addr
`endif


    /**
     * Instantiate C++ core
     */
    niosv_cpp_core #(
        .MXLEN                   ( MXLEN                 ),
        .HARTID                  ( HARTID                ),
        .ALULEN                  ( ALULEN                ),
        .ADR_LEN                 ( MXLEN                 ),
        .RESET_VECTOR            ( RESET_VECTOR          ),

        .DEBUG_ENA               ( DEBUG_ENA             ),
        .DEBUG_VECTOR            ( DEBUG_VECTOR          ),
        .PARKLOOP_VECTOR         ( PARKLOOP_VECTOR       ),

        .Zicsr_ENA               ( Zicsr_ENA             ),
        .Zicntr_ENA              ( Zicntr_ENA            ),

        .RVC_ENA                 ( RVC_ENA               ),
  
        .Zba_ENA                 ( Zba_ENA               ),
        .Zbb_ENA                 ( Zbb_ENA               ),
        .Zbc_ENA                 ( Zbc_ENA               ),
        .Zbs_ENA                 ( Zbs_ENA               ),

        .Zmmul_ENA               ( Zmmul_ENA             ),

        .ECC_ENA                 ( ECC_ENA               ),
        .FAST_SHIFT              ( FAST_SHIFT            ),
        .TARGET_AREA             ( TARGET_AREA           ),
        .DEVICE_FAMILY           ( DEVICE_FAMILY         ))
    core_inst (
        .rst_ni                  (~reset                 ),
        .clk_i                   ( clk                   ),
        .reset_req_i             ( reset_req             ),
        .reset_reqack_o          ( reset_req_ack         ),

        .extif_instruction_o     ( instruction           ),
        .extif_address_o         ( address               ),
        .extif_byteenable_o      ( byteenable            ),
        .extif_read_o            ( read                  ),
        .extif_readinstruction_i ( readinstruction       ),
        .extif_readdata_i        ( readdata              ),
        .extif_write_o           ( write                 ),
        .extif_writedata_o       ( writedata             ),
        .extif_done_i            ( done                  ),

        //interrupt
        .irq_debug_i             ( irq_debug             ),
        .irq_external_i          ( irq_ext               ),
        .irq_software_i          ( irq_sw                ),
        .irq_timer_i             ( irq_timer             ),
        .irq_platform_i          ( irq_plat_vec          ),

        .ecc_status_o            ( core_ecc_status       ),
        .ecc_src_o               ( core_ecc_src          )
//`ifdef RISCV_FORMAL
        ,
        .rvfi_valid              ( rvfi_valid            ),
        .rvfi_order              ( rvfi_order            ),
        .rvfi_insn               ( rvfi_insn             ),
        .rvfi_trap               ( rvfi_trap             ),
        .rvfi_halt               ( rvfi_halt             ),
        .rvfi_intr               ( rvfi_intr             ),
        .rvfi_mode               ( rvfi_mode             ),
        .rvfi_ixl                ( rvfi_ixl              ),
        .rvfi_rs1_addr           ( rvfi_rs1_addr         ),
        .rvfi_rs2_addr           ( rvfi_rs2_addr         ),
        .rvfi_rs1_rdata          ( rvfi_rs1_rdata        ),
        .rvfi_rs2_rdata          ( rvfi_rs2_rdata        ),
        .rvfi_rd_addr            ( rvfi_rd_addr          ),
        .rvfi_rd_wdata           ( rvfi_rd_wdata         ),
        .rvfi_rd_wren            ( rvfi_rd_wren          ),
        .rvfi_pc_rdata           ( rvfi_pc_rdata         ),
        .rvfi_pc_wdata           ( rvfi_pc_wdata         ),
        .rvfi_mem_addr           ( rvfi_mem_addr         ),
        .rvfi_mem_rmask          ( rvfi_mem_rmask        ),
        .rvfi_mem_wmask          ( rvfi_mem_wmask        ),
        .rvfi_mem_rdata          ( rvfi_mem_rdata        ),
        .rvfi_mem_wdata          ( rvfi_mem_wdata        )
//`endif
`ifdef NIOSV_DV
        ,
        .rs1_ecc_force_i         ( rs1_ecc_force         ),
        .rs1_ecc_status_o        ( rs1_ecc_status        ),
        .rs2_ecc_force_i         ( rs2_ecc_force         ),
        .rs2_ecc_status_o        ( rs2_ecc_status        )
`endif
);

    /**
     * Assign Interface signals
     */
    wire instr_done;
    wire data_done;
    niosv_cpp_axi4lite_if #(
      .MXLEN                     ( MXLEN                 ),
      .REGISTERED                ( INSTR_REGISTERED      ))
    instr_axi_if_inst (
      .rst_ni                    (~reset                 ),
      .clk_i                     ( clk                   ),

      //to/from core
      .extif_enable_i            ( instruction           ),
      .extif_instruction_i       ( 1'b1                  ),
      .extif_read_i              ( read                  ),
      .extif_write_i             ( 1'b0                  ),
      .extif_done_o              ( instr_done            ),
      .extif_address_i           ( address               ),
      .extif_byteena_i           ( {MXLEN/8{1'b1}}       ),
      .extif_rddata_o            ( readinstruction       ),
      .extif_wrdata_i            ( {MXLEN{1'b0}}         ),

      //Write Address
      .awaddr_o                  ( instr_awaddr          ),
      .awprot_o                  ( instr_awprot          ),
      .awvalid_o                 ( instr_awvalid         ),
      .awready_i                 ( 1'b0                  ),

      //Write Data
      .wdata_o                   ( instr_wdata           ),
      .wstrb_o                   ( instr_wstrb           ),
      .wvalid_o                  ( instr_wvalid          ),
      .wready_i                  ( 1'b0                  ),

      //Write Response
      .bresp_i                   ( 2'b00                 ), //BRESP_OKAY
      .bvalid_i                  ( 1'b0                  ),
      .bready_o                  ( instr_bready          ),

      //Read Address
      .araddr_o                  ( instr_araddr          ),
      .arprot_o                  ( instr_arprot          ),
      .arvalid_o                 ( instr_arvalid         ),
      .arready_i                 ( instr_arready         ),

      //Read Data
      .rdata_i                   ( instr_rdata           ),
      .rresp_i                   ( instr_rresp           ),
      .rvalid_i                  ( instr_rvalid          ),
      .rready_o                  ( instr_rready          ));

    niosv_cpp_axi4lite_if #(
      .MXLEN                     ( MXLEN                 ),
      .REGISTERED                ( DATA_REGISTERED       ))
    data_axi_if_inst (
      .rst_ni                    (~reset                 ),
      .clk_i                     ( clk                   ),

      //to/from core
      .extif_enable_i            (~instruction           ),
      .extif_instruction_i       ( 1'b0                  ),
      .extif_read_i              ( read                  ),
      .extif_write_i             ( write                 ),
      .extif_done_o              ( data_done             ),
      .extif_address_i           ( address               ),
      .extif_byteena_i           ( byteenable            ),
      .extif_rddata_o            ( readdata              ),
      .extif_wrdata_i            ( writedata             ),

      //Write Address
      .awaddr_o                  ( data_awaddr           ),
      .awprot_o                  ( data_awprot           ),
      .awvalid_o                 ( data_awvalid          ),
      .awready_i                 ( data_awready          ),

      //Write Data
      .wdata_o                   ( data_wdata            ),
      .wstrb_o                   ( data_wstrb            ),
      .wvalid_o                  ( data_wvalid           ),
      .wready_i                  ( data_wready           ),

      //Write Response
      .bresp_i                   ( data_bresp            ),
      .bvalid_i                  ( data_bvalid           ),
      .bready_o                  ( data_bready           ),

      //Read Address
      .araddr_o                  ( data_araddr           ),
      .arprot_o                  ( data_arprot           ),
      .arvalid_o                 ( data_arvalid          ),
      .arready_i                 ( data_arready          ),

      //Read Data
      .rdata_i                   ( data_rdata            ),
      .rresp_i                   ( data_rresp            ),
      .rvalid_i                  ( data_rvalid           ),
      .rready_o                  ( data_rready           ));
    assign done     = instruction ? instr_done : data_done;
endmodule



# qsys scripting (.tcl) file for system_soc
package require -exact qsys 16.0

create_system {system_soc}

set_project_property DEVICE_FAMILY {Cyclone IV E}
set_project_property DEVICE {EP4CE10E22C8}
set_project_property HIDE_FROM_IP_CATALOG {false}

# Instances and instance parameters
# (disabled instances are intentionally culled)
add_instance CLK clock_source 25.1
set_instance_parameter_value CLK {clockFrequency} {50000000.0}
set_instance_parameter_value CLK {clockFrequencyKnown} {1}
set_instance_parameter_value CLK {resetSynchronousEdges} {NONE}

add_instance CPU intel_niosv_c 4.1.0
set_instance_parameter_value CPU {archSize} {32}
set_instance_parameter_value CPU {enableAvalonInterface} {0}
set_instance_parameter_value CPU {enableECCLite} {0}
set_instance_parameter_value CPU {enableResetReq} {0}
set_instance_parameter_value CPU {hartID} {0}
set_instance_parameter_value CPU {numGpr} {32}
set_instance_parameter_value CPU {resetOffset} {0}
set_instance_parameter_value CPU {resetSlave} {Absolute}
set_instance_parameter_value CPU {useResetReq} {0}

add_instance I2C altera_avalon_i2c 25.1
set_instance_parameter_value I2C {FIFO_DEPTH} {32}
set_instance_parameter_value I2C {USE_AV_ST} {0}

add_instance SRAM altera_avalon_onchip_memory2 25.1
set_instance_parameter_value SRAM {allowInSystemMemoryContentEditor} {0}
set_instance_parameter_value SRAM {blockType} {AUTO}
set_instance_parameter_value SRAM {copyInitFile} {0}
set_instance_parameter_value SRAM {dataWidth} {32}
set_instance_parameter_value SRAM {dataWidth2} {32}
set_instance_parameter_value SRAM {dualPort} {0}
set_instance_parameter_value SRAM {ecc_enabled} {0}
set_instance_parameter_value SRAM {enPRInitMode} {0}
set_instance_parameter_value SRAM {enableDiffWidth} {0}
set_instance_parameter_value SRAM {initMemContent} {1}
set_instance_parameter_value SRAM {initializationFileName} {onchip_mem.hex}
set_instance_parameter_value SRAM {instanceID} {NONE}
set_instance_parameter_value SRAM {memorySize} {32768.0}
set_instance_parameter_value SRAM {readDuringWriteMode} {DONT_CARE}
set_instance_parameter_value SRAM {resetrequest_enabled} {1}
set_instance_parameter_value SRAM {simAllowMRAMContentsFile} {0}
set_instance_parameter_value SRAM {simMemInitOnlyFilename} {0}
set_instance_parameter_value SRAM {singleClockOperation} {0}
set_instance_parameter_value SRAM {slave1Latency} {1}
set_instance_parameter_value SRAM {slave2Latency} {1}
set_instance_parameter_value SRAM {useNonDefaultInitFile} {0}
set_instance_parameter_value SRAM {useShallowMemBlocks} {0}
set_instance_parameter_value SRAM {writable} {1}

add_instance TIMER altera_avalon_timer 25.1
set_instance_parameter_value TIMER {alwaysRun} {1}
set_instance_parameter_value TIMER {counterSize} {32}
set_instance_parameter_value TIMER {fixedPeriod} {1}
set_instance_parameter_value TIMER {period} {1}
set_instance_parameter_value TIMER {periodUnits} {MSEC}
set_instance_parameter_value TIMER {resetOutput} {0}
set_instance_parameter_value TIMER {snapshot} {0}
set_instance_parameter_value TIMER {timeoutPulseOutput} {0}
set_instance_parameter_value TIMER {watchdogPulse} {2}

add_instance UART altera_avalon_uart 25.1
set_instance_parameter_value UART {baud} {115200}
set_instance_parameter_value UART {dataBits} {8}
set_instance_parameter_value UART {fixedBaud} {1}
set_instance_parameter_value UART {parity} {NONE}
set_instance_parameter_value UART {simCharStream} {}
set_instance_parameter_value UART {simInteractiveInputEnable} {0}
set_instance_parameter_value UART {simInteractiveOutputEnable} {0}
set_instance_parameter_value UART {simTrueBaud} {0}
set_instance_parameter_value UART {stopBits} {1}
set_instance_parameter_value UART {syncRegDepth} {2}
set_instance_parameter_value UART {useCtsRts} {0}
set_instance_parameter_value UART {useEopRegister} {0}
set_instance_parameter_value UART {useRelativePathForSimFile} {0}

# exported interfaces
add_interface clk clock sink
set_interface_property clk EXPORT_OF CLK.clk_in
add_interface com_export conduit end
set_interface_property com_export EXPORT_OF I2C.i2c_serial
add_interface debug_export conduit end
set_interface_property debug_export EXPORT_OF UART.external_connection
add_interface reset reset sink
set_interface_property reset EXPORT_OF CLK.clk_in_reset

# connections and connection parameters
add_connection CLK.clk CPU.clk

add_connection CLK.clk I2C.clock

add_connection CLK.clk SRAM.clk1

add_connection CLK.clk TIMER.clk

add_connection CLK.clk UART.clk

add_connection CLK.clk_reset CPU.reset

add_connection CLK.clk_reset I2C.reset_sink

add_connection CLK.clk_reset SRAM.reset1

add_connection CLK.clk_reset TIMER.reset

add_connection CLK.clk_reset UART.reset

add_connection CPU.data_manager I2C.csr
set_connection_parameter_value CPU.data_manager/I2C.csr arbitrationPriority {1}
set_connection_parameter_value CPU.data_manager/I2C.csr baseAddress {0x8000}
set_connection_parameter_value CPU.data_manager/I2C.csr defaultConnection {0}

add_connection CPU.data_manager SRAM.s1
set_connection_parameter_value CPU.data_manager/SRAM.s1 arbitrationPriority {1}
set_connection_parameter_value CPU.data_manager/SRAM.s1 baseAddress {0x0000}
set_connection_parameter_value CPU.data_manager/SRAM.s1 defaultConnection {0}

add_connection CPU.data_manager TIMER.s1
set_connection_parameter_value CPU.data_manager/TIMER.s1 arbitrationPriority {1}
set_connection_parameter_value CPU.data_manager/TIMER.s1 baseAddress {0x8040}
set_connection_parameter_value CPU.data_manager/TIMER.s1 defaultConnection {0}

add_connection CPU.data_manager UART.s1
set_connection_parameter_value CPU.data_manager/UART.s1 arbitrationPriority {1}
set_connection_parameter_value CPU.data_manager/UART.s1 baseAddress {0x8060}
set_connection_parameter_value CPU.data_manager/UART.s1 defaultConnection {0}

add_connection CPU.instruction_manager SRAM.s1
set_connection_parameter_value CPU.instruction_manager/SRAM.s1 arbitrationPriority {1}
set_connection_parameter_value CPU.instruction_manager/SRAM.s1 baseAddress {0x0000}
set_connection_parameter_value CPU.instruction_manager/SRAM.s1 defaultConnection {0}

# interconnect requirements
set_interconnect_requirement {$system} {qsys_mm.clockCrossingAdapter} {HANDSHAKE}
set_interconnect_requirement {$system} {qsys_mm.enableEccProtection} {FALSE}
set_interconnect_requirement {$system} {qsys_mm.insertDefaultSlave} {FALSE}
set_interconnect_requirement {$system} {qsys_mm.maxAdditionalLatency} {1}

save_system {system_soc.qsys}

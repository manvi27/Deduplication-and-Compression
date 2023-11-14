############################################################
## This file is generated automatically by Vitis HLS.
## Please DO NOT edit it.
## Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
############################################################
open_project ESE5320_Project
add_files Server/stopwatch.h
add_files Server/server.h
add_files Server/server.cpp
add_files Server/encoder.h
add_files Server/encoder.cpp
add_files SHA_algorithm/SHA256.h
add_files SHA_algorithm/SHA256.cpp
add_files Test/LittlePrince.txt
add_files Server/Host.cpp
add_files Dedup/Dedup.h
add_files Dedup/Dedup.cpp
add_files Decoder/Decoder.cpp
add_files -tb Test/Testbench.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xczu3eg-sbva484-1-i}
create_clock -period 6.666666666666666667ns -name default
#source "./ESE5320_Project/solution1/directives.tcl"
csim_design
csynth_design
cosim_design
export_design -format ip_catalog

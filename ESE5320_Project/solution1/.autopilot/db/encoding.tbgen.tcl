set moduleName encoding
set isTopModule 1
set isCombinational 0
set isDatapathOnly 0
set isPipelined 0
set pipeline_type none
set FunctionProtocol ap_ctrl_hs
set isOneStateSeq 0
set ProfileFlag 0
set StallSigGenFlag 0
set isEnableWaveformDebug 1
set C_modelName {encoding}
set C_modelType { void 0 }
set C_modelArgList {
	{ s1 int 8 regular {pointer 0}  }
	{ length_r int 32 regular  }
	{ output_code int 8 regular {pointer 1}  }
	{ output_code_len int 32 unused  }
}
set C_modelArgMapList {[ 
	{ "Name" : "s1", "interface" : "wire", "bitwidth" : 8, "direction" : "READONLY", "bitSlice":[{"low":0,"up":0,"cElement": [{"cName": "s1","cData": "char","bit_use": { "low": 0,"up": 0},"cArray": [{"low" : 0,"up" : 0,"step" : 0}]}]}]} , 
 	{ "Name" : "length_r", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY", "bitSlice":[{"low":0,"up":0,"cElement": [{"cName": "length","cData": "int","bit_use": { "low": 0,"up": 0},"cArray": [{"low" : 0,"up" : 0,"step" : 0}]}]}]} , 
 	{ "Name" : "output_code", "interface" : "wire", "bitwidth" : 8, "direction" : "WRITEONLY", "bitSlice":[{"low":0,"up":0,"cElement": [{"cName": "output_code","cData": "char","bit_use": { "low": 0,"up": 0},"cArray": [{"low" : 0,"up" : 0,"step" : 0}]}]}]} , 
 	{ "Name" : "output_code_len", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY", "bitSlice":[{"low":0,"up":0,"cElement": [{"cName": "output_code_len","cData": "int","bit_use": { "low": 0,"up": 0},"cArray": [{"low" : 0,"up" : 0,"step" : 0}]}]}]} ]}
# RTL Port declarations: 
set portNum 11
set portList { 
	{ ap_clk sc_in sc_logic 1 clock -1 } 
	{ ap_rst sc_in sc_logic 1 reset -1 active_high_sync } 
	{ ap_start sc_in sc_logic 1 start -1 } 
	{ ap_done sc_out sc_logic 1 predone -1 } 
	{ ap_idle sc_out sc_logic 1 done -1 } 
	{ ap_ready sc_out sc_logic 1 ready -1 } 
	{ s1 sc_in sc_lv 8 signal 0 } 
	{ length_r sc_in sc_lv 32 signal 1 } 
	{ output_code sc_out sc_lv 8 signal 2 } 
	{ output_code_ap_vld sc_out sc_logic 1 outvld 2 } 
	{ output_code_len sc_in sc_lv 32 signal 3 } 
}
set NewPortList {[ 
	{ "name": "ap_clk", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "clock", "bundle":{"name": "ap_clk", "role": "default" }} , 
 	{ "name": "ap_rst", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "reset", "bundle":{"name": "ap_rst", "role": "default" }} , 
 	{ "name": "ap_start", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "start", "bundle":{"name": "ap_start", "role": "default" }} , 
 	{ "name": "ap_done", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "predone", "bundle":{"name": "ap_done", "role": "default" }} , 
 	{ "name": "ap_idle", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "done", "bundle":{"name": "ap_idle", "role": "default" }} , 
 	{ "name": "ap_ready", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "ready", "bundle":{"name": "ap_ready", "role": "default" }} , 
 	{ "name": "s1", "direction": "in", "datatype": "sc_lv", "bitwidth":8, "type": "signal", "bundle":{"name": "s1", "role": "default" }} , 
 	{ "name": "length_r", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "length_r", "role": "default" }} , 
 	{ "name": "output_code", "direction": "out", "datatype": "sc_lv", "bitwidth":8, "type": "signal", "bundle":{"name": "output_code", "role": "default" }} , 
 	{ "name": "output_code_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "output_code", "role": "ap_vld" }} , 
 	{ "name": "output_code_len", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "output_code_len", "role": "default" }}  ]}

set RtlHierarchyInfo {[
	{"ID" : "0", "Level" : "0", "Path" : "`AUTOTB_DUT_INST", "Parent" : "", "Child" : ["1", "2", "3", "4", "5", "6", "7"],
		"CDFG" : "encoding",
		"Protocol" : "ap_ctrl_hs",
		"ControlExist" : "1", "ap_start" : "1", "ap_ready" : "1", "ap_done" : "1", "ap_continue" : "0", "ap_idle" : "1", "real_start" : "0",
		"Pipeline" : "None", "UnalignedPipeline" : "0", "RewindPipeline" : "0", "ProcessNetwork" : "0",
		"II" : "0",
		"VariableLatency" : "1", "ExactLatency" : "-1", "EstimateLatencyMin" : "-1", "EstimateLatencyMax" : "-1",
		"Combinational" : "0",
		"Datapath" : "0",
		"ClockEnable" : "0",
		"HasSubDataflow" : "0",
		"InDataflowNetwork" : "0",
		"HasNonBlockingOperation" : "0",
		"Port" : [
			{"Name" : "s1", "Type" : "None", "Direction" : "I"},
			{"Name" : "length_r", "Type" : "None", "Direction" : "I"},
			{"Name" : "output_code", "Type" : "Vld", "Direction" : "O"},
			{"Name" : "output_code_len", "Type" : "None", "Direction" : "I"}]},
	{"ID" : "1", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.hash_table_U", "Parent" : "0"},
	{"ID" : "2", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.my_assoc_mem_upper_key_mem_U", "Parent" : "0"},
	{"ID" : "3", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.my_assoc_mem_middle_key_mem_U", "Parent" : "0"},
	{"ID" : "4", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.my_assoc_mem_lower_key_mem_U", "Parent" : "0"},
	{"ID" : "5", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.my_assoc_mem_value_U", "Parent" : "0"},
	{"ID" : "6", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.out_tmp_U", "Parent" : "0"},
	{"ID" : "7", "Level" : "1", "Path" : "`AUTOTB_DUT_INST.grp_insert_fu_696", "Parent" : "0",
		"CDFG" : "insert",
		"Protocol" : "ap_ctrl_hs",
		"ControlExist" : "1", "ap_start" : "1", "ap_ready" : "1", "ap_done" : "1", "ap_continue" : "0", "ap_idle" : "1", "real_start" : "0",
		"Pipeline" : "Aligned", "UnalignedPipeline" : "0", "RewindPipeline" : "0", "ProcessNetwork" : "0",
		"II" : "2",
		"VariableLatency" : "0", "ExactLatency" : "12", "EstimateLatencyMin" : "12", "EstimateLatencyMax" : "12",
		"Combinational" : "0",
		"Datapath" : "0",
		"ClockEnable" : "1",
		"HasSubDataflow" : "0",
		"InDataflowNetwork" : "0",
		"HasNonBlockingOperation" : "0",
		"Port" : [
			{"Name" : "hash_table", "Type" : "Memory", "Direction" : "IO"},
			{"Name" : "mem_upper_key_mem", "Type" : "Memory", "Direction" : "IO"},
			{"Name" : "mem_middle_key_mem", "Type" : "Memory", "Direction" : "IO"},
			{"Name" : "mem_lower_key_mem", "Type" : "Memory", "Direction" : "IO"},
			{"Name" : "mem_value", "Type" : "Memory", "Direction" : "O"},
			{"Name" : "mem_fill_read_5", "Type" : "None", "Direction" : "I"},
			{"Name" : "mem_fill_read", "Type" : "None", "Direction" : "I"},
			{"Name" : "key", "Type" : "None", "Direction" : "I"},
			{"Name" : "value_r", "Type" : "None", "Direction" : "I"}]}]}


set ArgLastReadFirstWriteLatency {
	encoding {
		s1 {Type I LastRead 6 FirstWrite -1}
		length_r {Type I LastRead 0 FirstWrite -1}
		output_code {Type O LastRead -1 FirstWrite 11}
		output_code_len {Type I LastRead -1 FirstWrite -1}}
	insert {
		hash_table {Type IO LastRead 10 FirstWrite 11}
		mem_upper_key_mem {Type IO LastRead 11 FirstWrite 12}
		mem_middle_key_mem {Type IO LastRead 11 FirstWrite 12}
		mem_lower_key_mem {Type IO LastRead 11 FirstWrite 12}
		mem_value {Type O LastRead -1 FirstWrite 11}
		mem_fill_read_5 {Type I LastRead 1 FirstWrite -1}
		mem_fill_read {Type I LastRead 1 FirstWrite -1}
		key {Type I LastRead 0 FirstWrite -1}
		value_r {Type I LastRead 1 FirstWrite -1}}}

set hasDtUnsupportedChannel 0

set PerformanceInfo {[
	{"Name" : "Latency", "Min" : "-1", "Max" : "-1"}
	, {"Name" : "Interval", "Min" : "0", "Max" : "0"}
]}

set PipelineEnableSignalInfo {[
	{"Pipeline" : "2", "EnableSignal" : "ap_enable_pp2"}
	{"Pipeline" : "4", "EnableSignal" : "ap_enable_pp4"}
]}

set Spec2ImplPortList { 
	s1 { ap_none {  { s1 in_data 0 8 } } }
	length_r { ap_none {  { length_r in_data 0 32 } } }
	output_code { ap_vld {  { output_code out_data 1 8 }  { output_code_ap_vld out_vld 1 1 } } }
	output_code_len { ap_none {  { output_code_len in_data 0 32 } } }
}

set busDeadlockParameterList { 
}

# RTL port scheduling information:
set fifoSchedulingInfoList { 
}

# RTL bus port read request latency information:
set busReadReqLatencyList { 
}

# RTL bus port write response latency information:
set busWriteResLatencyList { 
}

# RTL array port load latency information:
set memoryLoadLatencyList { 
}

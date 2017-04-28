#!/usr/bin/python
import os
import sys
import ConfigParser


class Command(object):
    """
    providing a data structure to
    conveniently convert the format from simulator output to the verilog format
    """
    def __init__(self, line):
        elements = line.split()
        assert len(elements) == 8, "each line has to be in the format of "\
                                   "clk cmd chan rank bankgroup bank row col"
        self.clk = int(elements[0])
        self.cmd = elements[1]
        self.chan = int(elements[2])
        self.rank = int(elements[3])
        self.bankgroup = int(elements[4])
        self.bank = int(elements[5])
        self.row = int(elements[6], 16)
        self.col = int(elements[7], 16)

    def get_ddr4_str(self):
        """
        get a command line for verilog model benchmark
        """
        if self.cmd == "activate":
            return "activate(.bg(%d), .ba(%d), .row(%d));\n" % (self.bankgroup, self.bank, self.row)
        elif self.cmd == "read":  # ap=0 no auto precharge, bc=1 NO burst chop, weird...
            return "read(.bg(%d), .ba(%d), .col(%d), ap(0), bc(1));\n" % \
                   (self.bankgroup, self.bank, self.col)
        elif self.cmd == "read_p":
            return "read(.bg(%d), .ba(%d), .col(%d), ap(1), bc(1));\n" % \
                   (self.bankgroup, self.bank, self.col)
        elif self.cmd == "write":
            return "write(.bg(%d), .ba(%d), .col(%d), ap(0), bc(1));\n" % \
                   (self.bankgroup, self.bank, self.col)
        elif self.cmd == "write_p":
            return "write(.bg(%d), .ba(%d), .col(%d), ap(1), bc(1));\n" % \
                   (self.bankgroup, self.bank, self.col)
        elif self.cmd == "precharge":
            return "precharge(.bg(%d), .ba(%d), ap(0));\n" % \
                   (self.bankgroup, self.bank)
        elif self.cmd == "refresh" or self.cmd == "refresh_bank":
            return "refresh();\n"  # currently the verilog model doesnt do bank refresh
        return


def get_val(config, sec, opt):
    """
    get value from a ini file given the section and option
    the priority here is int, float, boolean and finally string
    """
    try:
        val = config.getint(sec, opt)
    except ValueError:
        try:
            val = config.getfloat(sec, opt)
        except ValueError:
            try:
                val = config.getboolean(sec, opt)
            except ValueError:
                val = config.get(sec, opt)  # shouldn't have any exceptions here..
    return val

def get_config_dict(config_file):
    """
    read a ini file specified by config_file and
    return a dict of configs with [section][option] : value structure
    """
    _config_dict = {}
    config = ConfigParser.ConfigParser()
    config.read(config_file)
    for sec in config.sections():
        _config_dict[sec] = {}
        for opt in config.options(sec):
            _config_dict[sec][opt] = get_val(config, sec, opt)
    return _config_dict


def calculate_megs_per_device(config):
    """
    given config dict, calculate device density in MBs
    """
    rows = config["dram_structure"]["rows"]
    cols = config["dram_structure"]["columns"]
    width = config["dram_structure"]["device_width"]
    bgs = config["dram_structure"]["bankgroups"]
    banks_per_group = config["dram_structure"]["banks_per_group"]
    banks = bgs * banks_per_group
    bytes_per_bank = rows * cols * width / 8
    mega_bytes_per_device = bytes_per_bank * banks / 1024 /1024
    return mega_bytes_per_device

def get_ddr4_prefix_str(config):
    """
    this is necessary for setting up a verilog workbench
    depending on the config, some of the values in this string will change correspondingly
    """
    ts_table = {  # tCK -> [min_ts, nominal_ts, max_ts]
        1.875: ["TS_1875", "TS_1875", "TS_1875"],  # 1066MHz
        1.500: ["TS_1500", "TS_1500", "TS_1875"],  # 1333MHz
        1.250: ["TS_1250", "TS_1250", "TS_1500"],  # 1600MHz
        1.072: ["TS_1072", "TS_1072", "TS_1250"],  # 1866MHz
        0.938: ["TS_938", "TS_938", "TS_1072"],    # 2133MHz
        0.833: ["TS_833", "TS_833", "TS_938"],     # 2400MHz
        0.750: ["TS_750", "TS_750", "TS_833"],     # 2667MHz
        0.682: ["TS_682", "TS_682", "TS_750"],     # 2934MHz
        0.625: ["TS_625", "TS_625", "TS_682"]      # 3200MHz
    }
    ts = config["timing"]["tck"]
    if ts not in ts_table.keys():
        print "Invalid tCK value in ini file, use the followings for DDR4:" +\
               str([k for k in ts_table])
    ddr4_prefix_str = """
    initial begin : test
            UTYPE_TS min_ts, nominal_ts, max_ts;
            reg [MAX_BURST_LEN*MAX_DQ_BITS-1:0] b0to7, b8tof, b7to0, bfto8;
            reg [MODEREG_BITS-1:0] mode_regs[MAX_MODEREGS];
            UTYPE_DutModeConfig dut_mode_config;
            bit failure;
    min_ts = %s;
    nominal_ts = %s;
    max_ts = %s;
    b0to7 = { {MAX_DQ_BITS/4{4'h7}}, {MAX_DQ_BITS/4{4'h6}}, {MAX_DQ_BITS/4{4'h5}}, {MAX_DQ_BITS/4{4'h4}},
                {MAX_DQ_BITS/4{4'h3}}, {MAX_DQ_BITS/4{4'h2}}, {MAX_DQ_BITS/4{4'h1}}, {MAX_DQ_BITS/4{4'h0}} };
    b8tof = { {MAX_DQ_BITS/4{4'hf}}, {MAX_DQ_BITS/4{4'he}}, {MAX_DQ_BITS/4{4'hd}}, {MAX_DQ_BITS/4{4'hc}},
                {MAX_DQ_BITS/4{4'hb}}, {MAX_DQ_BITS/4{4'ha}}, {MAX_DQ_BITS/4{4'h9}}, {MAX_DQ_BITS/4{4'h8}} };
    b7to0 = { {MAX_DQ_BITS/4{4'h0}}, {MAX_DQ_BITS/4{4'h1}}, {MAX_DQ_BITS/4{4'h2}}, {MAX_DQ_BITS/4{4'h3}},
                {MAX_DQ_BITS/4{4'h4}}, {MAX_DQ_BITS/4{4'h5}}, {MAX_DQ_BITS/4{4'h6}}, {MAX_DQ_BITS/4{4'h7}} };
    bfto8 = { {MAX_DQ_BITS/4{4'h8}}, {MAX_DQ_BITS/4{4'h9}}, {MAX_DQ_BITS/4{4'ha}}, {MAX_DQ_BITS/4{4'hb}},
                {MAX_DQ_BITS/4{4'hc}}, {MAX_DQ_BITS/4{4'hd}}, {MAX_DQ_BITS/4{4'he}}, {MAX_DQ_BITS/4{4'hf}} };
    iDDR4.RESET_n <= 1'b1;
    iDDR4.CKE <= 1'b0;
    iDDR4.CS_n  <= 1'b1;
    iDDR4.ACT_n <= 1'b1;
    iDDR4.RAS_n_A16 <= 1'b1;
    iDDR4.CAS_n_A15 <= 1'b1;
    iDDR4.WE_n_A14 <= 1'b1;
    iDDR4.BG <= '1;
    iDDR4.BA <= '1;
    iDDR4.ADDR <= '1;
    iDDR4.ADDR_17 <= '0;
    iDDR4.ODT <= 1'b0;
    iDDR4.PARITY <= 0;
    iDDR4.ALERT_n <= 1;
    iDDR4.PWR <= 0;
    iDDR4.TEN <= 0;
    iDDR4.VREF_CA <= 0;
    iDDR4.VREF_DQ <= 0;
    iDDR4.ZQ <= 0;
    dq_en <= 1'b0;
    dqs_en <= 1'b0;
    default_period(nominal_ts);
    // POWERUP SECTION 
    power_up
    // Reset DLL
    dut_mode_config = _state.DefaultDutModeConfig(.cl(%d),
                                                    .write_recovery(%d),
                                                    .qoff(0),
                                                    .cwl(%d),
                                                    .wr_preamble_clocks(%d),
                                                    .bl_reg(rBLFLY),
                                                    .dll_enable(1),
                                                    .dll_reset(1));
    _state.ModeToAddrDecode(dut_mode_config, mode_regs);
    load_mode(.bg(0), .ba(1), .addr(mode_regs[1]));
    deselect(timing.tDLLKc); 
    dut_mode_config.DLL_reset = 0;
    _state.ModeToAddrDecode(dut_mode_config, mode_regs);
    load_mode(.bg(0), .ba(3), .addr(mode_regs[3]));
    deselect(timing.tMOD/timing.tCK);
    load_mode(.bg(1), .ba(2), .addr(mode_regs[6]));
    deselect(timing.tMOD/timing.tCK);
    load_mode(.bg(1), .ba(1), .addr(mode_regs[5]));
    deselect(timing.tMOD/timing.tCK);
    load_mode(.bg(1), .ba(0), .addr(mode_regs[4]));
    deselect(timing.tMOD/timing.tCK);
    load_mode(.bg(0), .ba(2), .addr(mode_regs[2]));
    deselect(timing.tMOD/timing.tCK);
    load_mode(.bg(0), .ba(1), .addr(mode_regs[1]));
    deselect(timing.tMOD/timing.tCK);
    load_mode(.bg(0), .ba(0), .addr(mode_regs[0]));
    deselect(timing.tMOD/timing.tCK);
    zq_cl();
    deselect(timing.tZQinitc);
    odt_out <= 1;                           // turn on odt
    """ % (ts_table[ts][0], ts_table[ts][1], ts_table[ts][2],
           config["timing"]["cl"], config["timing"]["twr"],
           config["timing"]["cwl"], config["timing"]["twpre"])
    return ddr4_prefix_str


def get_ddr4_postfix_str():
    postfix_str = """
        test_done;
    end
    """
    return postfix_str


def ddr4_trace_converter(trace_file_in, verilog_out):
    """
    convert the command trace file to the format that verilog model can include
    """
    with open(trace_file_in, "r") as trace_in:
        cmds = []
        for line in trace_in:
            cmds.append(Command(line))

        last_clk = 0
        for cmd in cmds:
            this_clk = cmd.clk
            nop_cycles = this_clk - last_clk - 1
            # fill nops in between actual commands
            if nop_cycles > 0:
                nop_str = "deselect(%d);\n" % nop_cycles
                verilog_out.write(nop_str)
            last_clk = this_clk
            verilog_out.write(cmd.get_ddr4_str())


def ddr3_validation(config, trace_file):
    return

def ddr4_validation(config, trace_file):
    """
    This function should:
    - generate a verilog file for the validation workbench
    - generate the command based on the config file
    """
    dev_str = "DDR4_"
    megs = calculate_megs_per_device(config)
    if megs == 512:
        density = "4G"
    elif megs == 1024:
        density = "8G"
    elif megs == 2048:
        density = "16G"
    else:
        print "unknown device density: %d! MBs" % (megs)
        exit(1)

    width = config["dram_structure"]["device_width"]

    dev_str = dev_str + density + "_X" + str(width)  # should be something like DDR4_8G_X8

    modelsim_cmd_str = "vlog -work work +acc -l vcs.log -novopt -sv "\
              "+define+%s arch_package.sv proj_package.sv " \
              "interface.sv StateTable.svp MemoryArray.svp ddr4_model.svp tb.sv" % dev_str
    print modelsim_cmd_str

    trace_out = trace_file+".vh"

    with open(trace_out, "wb") as vh_out:
        vh_out.write(get_ddr4_prefix_str(config))
        ddr4_trace_converter(trace_file, vh_out)
        vh_out.write(get_ddr4_postfix_str())
    return


if __name__ == "__main__":
    assert len(sys.argv) == 3, "Need 2 arguments, 1. config file name  2. command trace file name"

    if not (os.path.exists(sys.argv[1]) and os.path.exists(sys.argv[2])):
        print "cannot locate input files, please check your input file name and path"
        exit(1)

    ini_file = sys.argv[1]
    cmd_trace_file = sys.argv[2]

    configs = get_config_dict(ini_file)

    if configs["dram_structure"]["protocol"] == "DDR4":
        ddr4_validation(configs, cmd_trace_file)
    elif configs["dram_structure"]["protocol"] == "DDR3":
        ddr3_validation(configs, cmd_trace_file)
    else:
        pass






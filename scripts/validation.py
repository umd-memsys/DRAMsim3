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
        get a command line for verilog model workbench
        """
        if self.cmd == "activate":
            return "activate(.bg(%d), .ba(%d), .row(%d));\n" % (self.bankgroup, self.bank, self.row)
        elif self.cmd == "read":  # ap=0 no auto precharge, bc=1 NO burst chop, weird...
            return "read(.bg(%d), .ba(%d), .col(%d), .ap(0), .bc(1));\n" % \
                   (self.bankgroup, self.bank, self.col)
        elif self.cmd == "read_p":
            return "read(.bg(%d), .ba(%d), .col(%d), .ap(1), .bc(1));\n" % \
                   (self.bankgroup, self.bank, self.col)
        elif self.cmd == "write":
            return "write(.bg(%d), .ba(%d), .col(%d), .ap(0), .bc(1));\n" % \
                   (self.bankgroup, self.bank, self.col)
        elif self.cmd == "write_p":
            return "write(.bg(%d), .ba(%d), .col(%d), .ap(1), .bc(1));\n" % \
                   (self.bankgroup, self.bank, self.col)
        elif self.cmd == "precharge":
            return "precharge(.bg(%d), .ba(%d), .ap(0));\n" % \
                   (self.bankgroup, self.bank)
        elif self.cmd == "refresh" or self.cmd == "refresh_bank":
            return "refresh();\n"  # currently the verilog model doesnt do bank refresh
        return

    def get_ddr3_str(self):
        """
        get a command line for verilog model workbench
        """
        if self.cmd == "activate":
            return "activate(%d, %d);\n" % (self.bank, self.row)
        elif self.cmd == "read":
            return "read(%d, %d, %d, %d);\n" % (self.bank, self.col, 0, 1)
        elif self.cmd == "read_p":
            return "read(%d, %d, %d, %d);\n" % (self.bank, self.col, 1, 1)
        elif self.cmd == "write":
            return "write(%d, %d, %d, %d, %d, %d);\n" % (self.bank, self.col, 0, 1, 0, 0xdeadbeaf)
        elif self.cmd == "write_p":
            return "write(%d, %d, %d, %d, %d, %d);\n" % (self.bank, self.col, 1, 1, 0, 0xdeadbeaf)
        elif self.cmd == "refresh":
            return "refresh;\n"
        elif self.cmd == "precharge":
            return "precharge(%d, %d);\n" % (self.bank, 0)


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
    given config dict, calculate device density in Mbits
    """
    rows = config["dram_structure"]["rows"]
    cols = config["dram_structure"]["columns"]
    width = config["dram_structure"]["device_width"]
    bgs = config["dram_structure"]["bankgroups"]
    banks_per_group = config["dram_structure"]["banks_per_group"]
    banks = bgs * banks_per_group
    bytes_per_bank = rows * cols * width
    mega_bytes_per_device = bytes_per_bank * banks / 1024 /1024
    return mega_bytes_per_device


def get_ddr3_prefix_str():
    """
    nothing more than just setting up the workbench initialization
    """
    al = configs["timing"]["al"]
    cl = configs["timing"]["cl"]

    if al== 0:
        mod_1_str = "b0000000110"
    elif al == (cl - 1):
        mod_1_str = "b0000001110"
    elif al == (cl - 2):
        mod_1_str = "b0000010110"
    else:
        mod_1_str = "b0000000110"
        print "Invalid AL/CL values!"
        exit(1)

    print "CL:", al, " CL:", cl, " MOD Reg 1:" + mod_1_str

    prefix_str = """
    initial begin : test
        parameter [31:0] REP = DQ_BITS/8.0;
        reg         [BA_BITS-1:0] r_bank;
        reg        [ROW_BITS-1:0] r_row;
        reg        [COL_BITS-1:0] r_col;
        reg  [BL_MAX*DQ_BITS-1:0] r_data;
        integer                   r_i, r_j;
        real original_tck;
        reg [8*DQ_BITS-1:0] d0, d1, d2, d3;
        d0 = {
           {REP{8'h07}}, {REP{8'h06}}, {REP{8'h05}}, {REP{8'h04}},
           {REP{8'h03}}, {REP{8'h02}}, {REP{8'h01}}, {REP{8'h00}}
        };
        d1 = {
           {REP{8'h17}}, {REP{8'h16}}, {REP{8'h15}}, {REP{8'h14}},
           {REP{8'h13}}, {REP{8'h12}}, {REP{8'h11}}, {REP{8'h10}}
        };
        d2 = {
           {REP{8'h27}}, {REP{8'h26}}, {REP{8'h25}}, {REP{8'h24}},
           {REP{8'h23}}, {REP{8'h22}}, {REP{8'h21}}, {REP{8'h20}}
        };
        d3 = {
           {REP{8'h37}}, {REP{8'h36}}, {REP{8'h35}}, {REP{8'h34}},
           {REP{8'h33}}, {REP{8'h32}}, {REP{8'h31}}, {REP{8'h30}}
        };
        rst_n   <=  1'b0;
        cke     <=  1'b0;
        cs_n    <=  1'b1;
        ras_n   <=  1'b1;
        cas_n   <=  1'b1;
        we_n    <=  1'b1;
        ba      <=  {BA_BITS{1'bz}};
        a       <=  {ADDR_BITS{1'bz}};
        odt_out <=  1'b0;
        dq_en   <=  1'b0;
        dqs_en  <=  1'b0;
        // POWERUP SECTION 
        power_up;
        // INITIALIZE SECTION
        zq_calibration  (1);                            // perform Long ZQ Calibration
        load_mode       (3, 14'b00000000000000);        // Extended Mode Register (3)
        nop             (tmrd-1);
        load_mode       (2, {14'b00001000_000_000} | mr_cwl<<3); // Extended Mode Register 2 with DCC Disable
        nop             (tmrd-1);
        load_mode       (1, 14'%s);            // Extended Mode Register with DLL Enable
        nop             (tmrd-1);
        load_mode       (0, {14'b0_1_000_1_0_000_1_0_00} | mr_wr<<9 | mr_cl<<2); // Mode Register with DLL Reset
        nop             (683);  // make sure tDLLK and tZQINIT satisify
        odt_out         <= 0;                           // turn off odt, making life much easier...
        nop (7);  
        """ % (mod_1_str)
    return prefix_str


def get_ddr3_postfix_str():
    """
    end the workbench file
    """
    postfix_str = """
        test_done;
    end
    """
    return postfix_str


def ddr3_trace_converter(trace_file_in, verilog_out):
    with open(trace_file_in, "r") as trace_in:
        cmds = []
        for line in trace_in:
            cmds.append(Command(line))

        last_clk = 0
        for cmd in cmds:
            this_clk = cmd.clk
            nop_cycles = this_clk - last_clk - 1
            if nop_cycles > 0:
                nop_str = "nop(%d);\n" % nop_cycles
                verilog_out.write(nop_str)
            last_clk = this_clk
            verilog_out.write(cmd.get_ddr3_str())
    return


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
    power_up();
    // Reset DLL
    dut_mode_config = _state.DefaultDutModeConfig(.cl(%d),
                                                    .write_recovery(%d),
                                                    .qoff(0),
                                                    .cwl(%d),
                                                    .rd_preamble_clocks(%d),
                                                    .wr_preamble_clocks(%d),
                                                    .read_dbi(0),
                                                    .write_dbi(0),
                                                    .bl_reg(rBL8),
                                                    .dll_enable(1),
                                                    .dll_reset(1),
                                                    .dm_enable(0));
    dut_mode_config.AL = 0;
    dut_mode_config.BL = 8;
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
    odt_out <= 0;                           // turn off odt

    golden_model.SetDutModeConfig(dut_mode_config);
    golden_model.set_timing_parameter_lock(.locked(1), .recalculate_params(1), .non_spec_tck(%s)); // Prevent tCK changes from changing the loaded timeset.
    """ % (ts_table[ts][0],  # min_ts, not used
           ts_table[ts][1],
           ts_table[ts][2],  # max_ts, not used
           config["timing"]["cl"],
           config["timing"]["twr"] + 1,  # have to + 1 to satisfy the verilog model, wierd..
           config["timing"]["cwl"],
           config["timing"]["trpre"],
           config["timing"]["twpre"],
           ts_table[ts][1][3:],  # non_spec_tck, throw away TS_ and only leaves clock
          )
    return ddr4_prefix_str


def get_ddr4_postfix_str():
    """
    end the workbench file
    """
    return get_ddr3_postfix_str()


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
    """
    the command string should be something like:
    vlog +define+den2048Mb +define+x8 +define+sg15 tb.v ddr3.v
    also generate a trace_file.vh file that can be included in verilog workbench
    """
    megs = calculate_megs_per_device(config)
    if megs == 1024:
        density = "den1024Mb"
    elif megs == 2048:
        density = "den2048Mb"
    elif megs == 4096:
        density = "den4096Mb"
    elif megs == 8192:
        density = "den8192Mb"
    else:
        print "unknown device density: %d! MBs" % (megs)
        exit(1)
    width = config["dram_structure"]["device_width"]
    tck = config["timing"]["tck"]
    speed_table = {  # based on 1Gb device
        0.938: "sg093",  # 2133
        1.07 : "sg107",  # 1866
        1.25 : "sg125", # 1600
        1.50 : "sg15",  # 1333J, there is also sg15E
        1.875: "sg187", # 1066G, there is also sg187E
        2.5:   "sg25",  # 800, there is also sg25E
    }
    if tck not in speed_table.keys():
        print "Invalid tCK value in ini file, use the followings for DDR3:" +\
               str([k for k in speed_table])
    speed = speed_table[tck]

    cmd_str = "vlog +define+%s +define+x%d +define+%s tb.v ddr3.v" % (density, width, speed)
    print cmd_str

    trace_out = trace_file + ".vh"
    with open(trace_out, "wb") as vh_out:
        vh_out.write(get_ddr3_prefix_str())
        ddr3_trace_converter(trace_file, vh_out)
        vh_out.write(get_ddr3_postfix_str())
    return


def ddr4_validation(config, trace_file):
    """
    This function should:
    - generate a verilog file for the validation workbench
    - generate the command based on the config file
    """
    dev_str = "DDR4_"
    megs = calculate_megs_per_device(config)
    if megs == 4096:
        density = "4G"
    elif megs == 8192:
        density = "8G"
    elif megs == 16384:
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






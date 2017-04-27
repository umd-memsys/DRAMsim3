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


def ddr4_trace_converter(trace_file_in, verilog_file_out):
    with open(trace_file_in, "r") as trace_in, open(verilog_file_out, "wb") as trace_out:
        cmds = []
        for line in trace_in:
            cmds.append(Command(line))

        last_clk = 0
        for cmd in cmds:
            this_clk = cmd.clk
            nop_cycles = this_clk - last_clk
            # fill nops in between actual commands
            if nop_cycles > 0:
                nop_str = "deselect(%d)\n" % nop_cycles
                trace_out.write(nop_str)
            last_clk = this_clk

            trace_out.write(cmd.get_ddr4_str())




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
              "+define+DDR4_8G_X8 arch_package.sv proj_package.sv " \
              "interface.sv StateTable.svp MemoryArray.svp ddr4_model.svp tb.sv"
    print "Command String:"
    print modelsim_cmd_str
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






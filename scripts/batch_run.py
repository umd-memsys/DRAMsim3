#!/usr/bin/python

"""
Run a batch of configs
"""

import argparse
import os
import shlex
import subprocess
import sys
import tempfile
import parse_config

def process_configs(args):
    """given parsed args, return the config files in a list"""
    configs = []
    if args.configs:
        for c in args.configs:
            if not os.path.exists(c):
                print "config file ", c, " not exists!"
                exit(1)
            else:
                configs.append(c)
            
    if args.input_dir:
        if not os.path.exists(args.input_dir):
            print "input dir not exists!"
            exit(1)
        for f in os.listdir(args.input_dir):
            if f[-4:] != ".ini":
                print "INFO:ignoring non-ini file:", f
            else:
                configs.append(os.path.join(args.input_dir, f))

    return configs


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a batch of simulation"
                                     "with a given set of config files"
                                     "and possibliy redirect output, -h for help")
    parser.add_argument("executable", help="executable location")
    parser.add_argument("-o", "--output-dir", nargs="?", default="", help="override the 'output-prefix'"
                        "and the outputs will be 'output-dir/config-name-post-fix'"
                        "this won't mess the original file by using tempfiles")
    parser.add_argument("-c", "--configs", nargs="*", help="config files"
                        "NOTE we're assuming the hmc config has 'hmc' in it")
    parser.add_argument("-d", "--input-dir", help="input directory with configs")
    parser.add_argument("-n", default=100000, type=int, help="num of cycles, applied to all configs")
    parser.add_argument("--cpu-type", default="random", choices=["random", "trace", "stream"],
                        help="cpu type")
    parser.add_argument("-t", "--trace-file", help="trace file, only for trace cpu")
    args = parser.parse_args()

    # some input sanity check
    if not os.path.exists(args.executable):
        print "Exicutable: ", args.executable, " not exits!"
        exit(1)

    if not (args.input_dir or args.configs):
        print "Please provide config files with -o or -d option, see -h for help"
        exit(1)

    if args.input_dir and args.configs:
        print "WARNING: both -c and -d specified, using both..."

    configs = process_configs(args)   
    print configs

    if args.output_dir:
        if not os.path.exists(args.output_dir):
            try:
                os.mkdir(args.output_dir)
            except OSError:
                print "cannot make directory: ", args.output_dir
                exit(1)
            else:
                print "output dir not exists, creating one..."
        print "Overriding the output directory to: ", args.output_dir
    
    if args.cpu_type == "trace":
        if not os.path.exists(args.trace_file):
            print "trace file not found for trace cpu"
    
    for c in configs:
        mem_type = "default"
        if "hmc" in c.lower():
            mem_type = "hmc"

        config_file = c
        if args.output_dir:
            pure_config_name = os.path.basename(c)[:-4]
            new_prefix = os.path.join(args.output_dir, pure_config_name)
            temp_fp = parse_config.sub_options(c, "other", "output_prefix", new_prefix)
            config_file = temp_fp.name

        cmd_str = "%s --memory-type=%s --cpu-type=%s -c %s -n %d" % \
                  (args.executable, mem_type, args.cpu_type, config_file, args.n)
        
        if args.cpu_type == "trace":
            cmd_str += " --trace-file=%s" % args.trace_file

        print cmd_str
        try:
            subprocess.call(shlex.split(cmd_str))
        except KeyboardInterrupt:
            print "skipping this one..."


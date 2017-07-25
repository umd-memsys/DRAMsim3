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
    config_dirs = []
    if not args.input:
        print "please specify at least one input from -i option"
        exit(1)
        
    for item in args.input:
        if not os.path.exists(item):
            print "Input ", item, " not exists!"
            exit(1)

        if os.path.isfile(item):
            if item[-4:] != ".ini":
                print "INFO: ignoring non-ini file:", item
            else:
                configs.append(item)
        elif os.path.isdir(item):
            for f in os.listdir(item):
                if f[-4:] != ".ini":
                    print "INFO: ignoring non-ini file:", item
                else:
                    configs.append(os.path.join(item, f))
        else:
            print "???"
            exit(1)

    return configs


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a batch of simulation"
                                     "with a given set of config files"
                                     "and possibliy redirect output, -h for help")
    parser.add_argument("executable", help="executable location")
    parser.add_argument("-o", "--output-dir", nargs="?", default="", help="override the 'output-prefix'"
                        "and the outputs will be 'output-dir/config-name-post-fix'"
                        "this won't mess the original file by using tempfiles")
    parser.add_argument("-i", "--input", nargs="+", help="configs input, could be a dir or list of files"
                        "non-ini files will be ignored")
    parser.add_argument("-n", default=100000, type=int, help="num of cycles, applied to all configs")
    parser.add_argument("--cpu-type", default="random", choices=["random", "trace", "stream"],
                        help="cpu type")
    parser.add_argument("-t", "--trace-file", help="trace file, only for trace cpu")
    parser.add_argument("-q", "--quiet", action="store_true", help="Silent output" )
    args = parser.parse_args()

    # some input sanity check
    if not os.path.exists(args.executable):
        print "Exicutable: ", args.executable, " not exits!"
        exit(1)

    configs = process_configs(args)   
    # print configs

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
        temp_stdout = None
        if args.quiet:
            temp_stdout = tempfile.TemporaryFile()
        else:
            temp_stdout = sys.stdout
        try:
            subprocess.call(shlex.split(cmd_str), stdout=temp_stdout)
        except KeyboardInterrupt:
            print "skipping this one..."


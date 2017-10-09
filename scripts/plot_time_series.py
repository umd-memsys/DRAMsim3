#!/usr/bin/python
"""
Generate time series graphs of power/bandwidth/energy...
"""

import argparse
import os
import sys
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def plot_line(x, y, x_label, y_label, save_to=""):
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.plot(x, y)
    ax.set_xlim(0)
    ax.set_ylim(0)
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)
    ax.set_title(y_label)
    if save_to:
        file_name = y_label+"-vs-"+x_label+".png"
        fig.savefig(os.path.join(save_to, file_name))
    else:
        fig.show()


def plot_epochs(csv_file_name, serie_name, output_dir):
    """
    plot the time series of a specified stat serie (e.g. bw, power, etc)
    """
    df = pd.read_csv(csv_file_name)
    if not output_dir:
        output_dir = os.path.dirname(csv_file_name)
    if serie_name not in df:
        print("Serie %s not in %s!" % (serie_name, csv_file_name))
        exit(1)
    x_label = "Cycles"
    y_data = [0] + df[serie_name].tolist()
    cycles = df[x_label].tolist()
    x_ticks = [0]
    for i, epoch in enumerate(cycles):
        x_ticks.append(x_ticks[i] + epoch)
    plot_line(x_ticks, y_data, x_label, serie_name, output_dir)
    return


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot time serie graphs from "
                                     "stats csv outputs, type -h for more options")
    parser.add_argument("csv", help="epoch-stats csv file")
    parser.add_argument("-o", "--output-dir", help="output directory where the graphs will be"
                        "if not specified the output goes to the same dir as csv is in")
    parser.add_argument("-m", "--metric", help="metrics to be plotted, e(nergy), p(power),"
                        "and/or b(andwidth)")
    args = parser.parse_args()
    metrics = []
    if not args.metric:
        metrics = ["total_energy", "average_power", "average_bandwidth"]
    else:
        if "e" in args.metric:
            metrics.append("total_energy")
        if "p" in args.metric:
            metrics.append("average_power")
        if "b" in args.metric:
            metrics.append("average_bandwidth")

    for metric in metrics:
        plot_epochs(sys.argv[1], metric, args.output_dir)

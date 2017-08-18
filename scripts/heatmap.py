#!/usr/bin/python

from collections import OrderedDict

try:
    import numpy as np
    import pandas as pd
    import matplotlib
except:
    print "please install numpy, matplotlib and pandas"
    raise

matplotlib.use("Agg")
import matplotlib.pyplot as plt

def construct_mesh(df, val_key, xkey="x", ykey="y", zkey="z"):
    """
    to construct mesh data structure to be used for plotting later
    this should be as generic as possible
    the 4 keys in the args are used to locate data in the dataframe
    return a list of dicts that each dict represent a "z" layer (plane) of data
    """
    x = df[xkey]
    y = df[ykey]
    z = df[zkey]
    t = df[val_key]
    res = []  # for each z we return a group of xx, yy, tt
    for each_z, group in df.groupby(zkey):
        xx, yy = np.meshgrid(x.unique(), y.unique())
        # the data structure is matlib-like y-axis oriented
        y_groups = df.groupby(ykey)
        tt = []
        for i in y.unique():
            group = y_groups.get_group(i)
            t = group[val_key]
            tt.append(t)
        tt = np.array(tt)
        res.append({xkey:xx, ykey:yy, val_key:tt})
    return res


def plot_heatmap(x, y, t, title, save_to=""):
    """
    quick access to single heatmap
    """
    fig = plt.figure()
    ax = fig.add_subplot(111)
    pcm = ax.pcolormesh(x, y, t, cmap="RdYlGn_r")
    fig.colorbar(pcm, ax=ax, extend="max")
    fig.savefig("heatmap.png")

def plot_sub_heatmap(fig, ax, x, y, t, title):
    pcm = ax.pcolormesh(x, y, t, cmap="RdYlGn_r")
    fig.colorbar(pcm, ax=ax, extend="max")
    return fix, ax

def prep_fig_axes(num_plots, separate_figs=False):
    """
    if separate_figs is False then use subplots for each graph
    otherwise use different figs
    returns a dict with fig as keys and axes list as values
    """
    figs = OrderedDict()
    if not separate_figs:
        fig = plt.figure()
        axes = []
        if num_plots == 1:
            n_rows = 1
            n_cols = 1
        elif num_plots == 2:
            n_rows = 1
            n_cols = 2
        elif num_plots == 4:
            n_rows = 2
            n_cols = 2
        elif num_plots ==8:
            n_rows = 2
            n_cols = 4
        else:
            n_rows = 1
            n_cols = 1
        # create axes in subplot
        plot_num = 1
        for j in range(n_rows):
            for k in range(n_cols):
                axes.append(fig.add_subplot(j, k, plot_num))
                plot_num += 1
        figs[fig] = axes
    else:
        for i in range(num_plots):
            fig = plt.figure()
            ax = figure.add_subplot(111)
            figs[fig] = [ax]
    return figs

def plot_multi_heatmap(multi_layer_data, separate_figs=False, save_to=""):
    """
    this should also be a generic plot function 
    return figs and axes so that we can process it later
    """
    num_plots = len(multi_layer_data)
    figs = prep_fig_axes(num_plots, separate_figs=False)
    plot_num = 0
    for fig, axes in figs.iteritems():
        for ax in axes:
            data = multi_layer_data[plot_num]
            plot_sub_heatmap(fig, ax, data["x"], data["y"], data["val"], data["title"])
    return figs 


def plot_temp(csv_file_name):
    frame = pd.read_csv(csv_file_name)
    temp_data_all = construct_mesh(frame, "temperature")
    for i, temp_data in enumerate(temp_data_all):
        # iterate all layers
        title = "heatmap-layer-" + str(i)
        plot_heatmap(temp_data["x"], temp_data["y"], temp_data["val"], 
                     title, save_to=title+".png")

def plot_power(csv_file_name):
    frame = pd.read_csv(csv_file_name)
    temp_data_all = construct_mesh(frame, "power")
    for i, temp_data in enumerate(temp_data_all):
        # iterate all layers
        title = "heatmap-layer-" + str(i)
        plot_heatmap(temp_data["x"], temp_data["y"], temp_data["val"], 
                     title, save_to=title+".png")


def plot_simulation(stats_csv_file, bank_pos_file):
    frame = pd.read_csv(stats_csv_file)
    rank_frames = frame.groupby("rank_channel_index")
    power_data = []
    temp_data = []
    for rank, rank_frame in rank_frames:
        rank_power_data = construct_mesh(rank_frame, "power", "x", "y", "z")
        rank_temp_data = construct_mesh(rank_frame, "temperature", "x", "y", "z")
        for d in rank_power_data:
            d["title"] = "channel_rank_" + str(rank) + "_power_map"
        for d in rank_power_data:
            d["title"] = "channel_rank_" + str(rank) + "_temperature_map"

        power_data += rank_power_data
        temp_data += rank_temp_data

    power_figs = plot_multi_heatmap(power_data)
    temp_figs = plot_multi_heatmap(temp_data)

    # TODO add bank partition lines
    # TODO save figs
    



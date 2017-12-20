#!/usr/bin/python

"""
Analysis script that collect & visualize results, requires
- pandas
- matplotlib
"""

import pandas as pd
import matplotlib

def get_summary_df(stats_csvs, index=None):
    """
    given a bunch of csv files and concat all of them to a dataframe
    """
    frames = []
    for c in stats_csvs:
        df = pd.read_csv(c)
        frames.append(df)
    return pd.concat(frames)




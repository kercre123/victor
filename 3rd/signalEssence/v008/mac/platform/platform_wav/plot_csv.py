#!/usr/bin/env python

import scipy.io.wavfile
import os
import numpy as np
import matplotlib.pyplot as plt
import scipy.signal
import sys
import csv


# 2016 Signal Essence
# Robert Yu

"""
read and plot one or more diagnostic csv data files
usage:
python plot.csv a.csv [b.csv c.csv ...]

"""

def read_csv(fname):
    """reads a csv file, returns a numpy array"""
    row_index=0
    is_first_row = True
    csv_file=open(fname)
    csv_reader=csv.reader(csv_file,delimiter=',')
    num_cols = 0
    num_rows =  0
    for row in csv_reader:
        row_flt =np.float_(row)  
        
        if is_first_row:
            num_cols = row_flt.shape[0]

            # y.shape = (1, N)  (where N is at least 2: block index, value)
            y = np.reshape(row_flt, [1, num_cols])
            is_first_row=False
        else:
            row_flt = np.reshape(row_flt, [1, num_cols])

            # y.shape = (rows, N)
            y = np.concatenate((y, row_flt),axis=0)
        num_rows = num_rows+1
    print("read (%d) blocks, (%d) values per block" % (num_rows, num_cols-1))
    csv_file.close()
    
    return y

def plot(x,title="",total_plots=1, plot_index=1):
    assert(total_plots > 0)
    assert(plot_index>0)

    yrange = np.abs(np.max(x) - np.min(x))
    ylim_upper = np.max(x) + 0.1 * yrange;
    ylim_lower = np.min(x) - 0.1 * yrange;

    if total_plots==1:
        plt.plot(x)
        #plt.ylim(ylim_upper, ylim_lower)
        plt.title(title)
        plt.grid(True)
    else:
        plt.subplot(total_plots,1,plot_index)  # stack plots vertically
        plt.plot(x)
        #plt.ylim(ylim_upper, ylim_lower)
        plt.title(title)
        plt.grid(True)

def compute_ts_power_stats(x):
    """compute power statistics of time series
    
    return [max_pwr_db, min_pwr_db, mean_pwr_db, median_pwr_db]
    """
    pwr_x_db = 20.0 * np.log10(np.abs(x) + 0.00001)

    max_pwr_db = np.amax(pwr_x_db)
    min_pwr_db = np.amin(pwr_x_db)
    mean_pwr_db = np.mean(pwr_x_db)
    median_pwr_db = np.median(pwr_x_db)

    print("max instant power = %6.2f dB" % max_pwr_db)
    print("min instant power = %6.2f dB" % min_pwr_db)
    print("mean power = %6.2f dB" % mean_pwr_db)
    print("median power = %6.2f dB" % median_pwr_db)    

def append_legend(legend_list, fname, num_cols):
    """
    given csv filename and number of columns per row,
    return an appropriate list of legend labels
    """
    if num_cols==1:
        legend_list.append(fname)
    else:
        for col in range(num_cols):
            fname_plus_col = fname + ":" + str(col)
            legend_list.append(fname_plus_col)
    return legend_list
        

def print_signal_stats(x, varname):
    """
    do time-series statistical analysis on each column of data
    """
    print("signal stats for %s" % varname)
    for col in range(x.shape[1]):
        print("\n === col %d ===" % col)
        subset_x = x[:,col]
        compute_ts_power_stats(subset_x)

if __name__=="__main__":
    num_csv = len(sys.argv)-1
    print("%d csv files specified" % num_csv)

    fig, ax = plt.subplots()
    legend_label_list = []
    for n in range(1, len(sys.argv)):
        print("reading csv file %s" % sys.argv[n])
        x = read_csv(sys.argv[n])
        print(x.shape)

        # strip off first column, which is merely block index
        x = x[:,1:x.shape[1]]
        print(x.shape)
        ax.plot(x,'-',alpha=0.5)

        legend_label_list = append_legend(legend_label_list, sys.argv[n], x.shape[1])

        print_signal_stats(x, sys.argv[n])

        # plot(x[:,1],title=sys.argv[n],total_plots=num_csv, plot_index=n)
    plt.grid(True)
    plt.legend(legend_label_list)
    plt.show()


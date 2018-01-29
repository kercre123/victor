#!/usr/bin/env python3

#pip3 install plotly
#pip3 install colorlover

from collections import OrderedDict
import datetime
import fnmatch
import glob
import json
import logging
import os
import plotly
import plotly.graph_objs as go
import shutil
import sys
import argparse
import colorlover

# set up default logger
Logger = logging.getLogger('wifi_test')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(levelname)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)


def create_graph_from_files(args):
    performance_log_files = []

    if args.log_file == '':
        file_pattern = os.path.join(args.logs_dir, '**', 'json*', '*.log')
    else:
        file_pattern = os.path.join(args.logs_dir, '**', 'json*', args.log_file)

    for log_file in glob.iglob(file_pattern, recursive=True):
        performance_log_files.append(log_file)

    Logger.debug("Performance Log Files {}".format(performance_log_files))

    # { 'PerformanceFile1': [
    #     'room1': 'bps',
    #     'room2': 'bps',
    #   ],
    #   'PerformanceFile2': [
    #     'room1': 'bps',
    #     'room2': 'bps',
    #   ],
    # }
    results = {}
    for file_loc in performance_log_files:
        Logger.debug("Performance Log File {}".format(file_loc))
        file = open(file_loc)
        found_start = False
        json_data = "{"
        with open(file_loc) as file:
            for line in file:
                Logger.debug(line)
                if not found_start:
                    found_start = line[0] == "{"
                else:
                    json_data += line
        Logger.debug(json_data)
        file_json = json.loads(json_data)
        data_point_list = list({(0,0)})
        for interval in file_json['intervals']:
            data_point_list.append( (interval['sum']['end'],interval['sum']['bits_per_second']) )
        results[file_loc] = sorted(data_point_list, key=lambda data_point: data_point[0])
        file.close()
    Logger.debug(results)
    generate(results)


def generate(results_dict):
    create_traces_and_plot(results_dict)

def create_traces_and_plot(results_dict):
    traces = []
    for file, results_data in results_dict.items():
        traces.append(generate_2d_trace(results_data, file))

    plot(traces)


def print_heading(heading):
    Logger.info("")
    Logger.info("{}".format(heading))
    Logger.info("")


def print_seperator():
    Logger.info("_______")


def generate_2d_trace(results_data, file, map2=None):
    xs = []
    ys = []


    for plot_point in results_data:
        ys.append(plot_point[1]) #rate in bps
        xs.append(plot_point[0]) #time in seconds

    RdYlGn = colorlover.scales['8']['div']['RdYlGn']
    #bupu500 = colorlover.interp( RdYlGn, 500 ) # Map color scale to 500 bins
    Logger.info('###### {} Ys total: {}'.format("This Label", len(ys)))
    trace = go.Scatter(
        x=xs,
        y=ys,
        mode='lines',
        marker=dict(
            size=8,
            symbol='square',
            line=dict(width=1)
            ),
        name=file,
        text=file,
    )

    return trace


def plot(traces):
    data = traces
    layout = go.Layout(
        title='<b>WiFi Connection Rates</b>',
        hovermode='closest',
        xaxis=dict(
            title='Time (seconds)',
            categoryorder="category ascending",
            zeroline=False,
            gridwidth=2,
        ),
        margin=go.Margin(
            l=100,
            b=200,
        ),
        yaxis=dict(
            title='Rate (bits per second)',
            gridwidth=2,
        ),
    )
    fig = go.Figure(data=data, layout=layout)
    plotly.offline.plot(fig, filename='plot.html')


####################################
############ ARGUMENTS #############
####################################

def parse_args(argv=[], print_usage=False):
    parser = argparse.ArgumentParser()

    parser.add_argument('--debug', '-d', '--verbose', dest='debug', action='store_true',
                        help='prints debug output with various directory names and all values aggregated')
    parser.add_argument('--logs_dir',
                        help='directory of log files clips', action='store')
    parser.add_argument('--log_file',
                        help='Log File name', action='store', default='')

    if print_usage:
        parser.print_help()
        sys.exit(2)

    args = parser.parse_args(argv)

    if (args.debug):
        print(args)
        Logger.setLevel(logging.DEBUG)
    else:
        Logger.setLevel(logging.INFO)

    return args


####################################
############### MAIN ###############
####################################

def run(args):
    if os.path.isdir('output'):
        shutil.rmtree('output')
    os.mkdir('output')

    Logger.info('## Starting ############# {} #################'.format(datetime.datetime.now()))
    #generate_from_file(args.json_outfile)
    create_graph_from_files(args)
    Logger.info('## Finished ############# {} #################'.format(datetime.datetime.now()))

if __name__ == '__main__':
    args = parse_args(sys.argv[1:])
    sys.exit(run(args))

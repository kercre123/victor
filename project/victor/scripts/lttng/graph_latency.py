#!/usr/bin/env python3
import argparse
import numpy as np
import matplotlib.pyplot as plt
import bisect
import pickle

colors=['r','b','g','m','c','k']
traces = {}

fix, (ax1) = plt.subplots(nrows=1,ncols=1, figsize=(8,8))
fix.tight_layout()

parser = argparse.ArgumentParser(description='Graph vic-robot time between robot tick ending and starting')
parser.add_argument('-f', '--file', help='Path of file to load trace from', metavar='f')
args = parser.parse_args()

with open(args.file, 'rb') as handle:
    traces = pickle.load(handle)

    robot_loop_periods = []
    for k,v in traces.items():
        events = traces[k]
        for event in events:
            if event['name'] == 'anki_ust:vic_robot_robot_loop_period':
                robot_loop_periods.append(event['delay'])

    robot_loop_periods.sort()
    # The value 7500 is from cosmoBot.cpp:MAIN_TOO_LATE_TIME_THRESH_USEC = ROBOT_TIME_STEP_MS * 1500
    # where ROBOT_TIME_STEP_MS is 5
    toolate_index = bisect.bisect_left(robot_loop_periods, 7500)
    numevents = len(robot_loop_periods)
    percentiles = [0.0,90.0,99.0,99.9,99.99,99.999, 100]

    indxes = np.arange(len(percentiles))
    values = np.array(list(map(lambda x: np.percentile(robot_loop_periods,x), percentiles)))

    ax1.xaxis.set_ticks(percentiles)
    ax1.xaxis.set_ticklabels(percentiles)
    ax1.plot(indxes,values, colors.pop(0), label=k)

    ax1.xlabel=("Percentile")
    ax1.ylabel=("us")

    ax1.xaxis.set_ticks(indxes)
    ax1.xaxis.set_ticklabels(percentiles)
    ax1.legend()

plt.xlabel("Percentile")
plt.ylabel("Microseconds")
plt.grid(True)
plt.show()

#!/usr/bin/env python3
import argparse
import babeltrace
import os
import sys
import csv
import pickle

trace_handle = []
traces = {}

parser = argparse.ArgumentParser(description='Pickle LTTNG trace')
parser.add_argument('-f', '--file', help='Path of file to save trace to', metavar='f')
parser.add_argument('-t', '--trace', help='Path to trace directories to pickle', nargs='+')
args = parser.parse_args()

for x in args.trace:
    trace_collection = babeltrace.TraceCollection()
    for trace_pid in os.listdir("%s/ust/uid/" % x):
        trace_collection.add_trace("%s/ust/uid/%s/32-bit" % (x,trace_pid), 'ctf')
        trace_handle.append(trace_collection)
    traces[x] = []
    for event in trace_collection.events:
        event_map = {'name':event.name}
        for v in event:
            event_map[v] = event[v]
        traces[x].append(event_map)

with open(args.file, 'wb') as handle:
    pickle.dump(traces, handle)

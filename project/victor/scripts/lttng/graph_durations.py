#!/usr/bin/env python3
import babeltrace
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import sys
import csv


# get the trace path from the first command line argument
trace_path = sys.argv[1]

trace_collection = babeltrace.TraceCollection()

trace_handle = trace_collection.add_trace(trace_path, 'ctf')

anki_events = {'anki_ust:vic_robot_loop_duration': [],'anki_ust:vic_anim_loop_duration': [], 'anki_ust:vic_engine_loop_duration': []}
for event in trace_collection.events:
    if event.name in anki_events:
        anki_events[event.name].append(event['duration'])

fix, (ax1, ax2,ax3) = plt.subplots(nrows=3,ncols=1, figsize=(4,8))
ax1.hist(anki_events['anki_ust:vic_robot_loop_duration'], bins='auto')
ax1.set_title("vic_robot_loop_duration")

ax2.hist(anki_events['anki_ust:vic_anim_loop_duration'], bins='auto')
ax2.set_title("vic_anim_loop_duration")

ax3.hist(anki_events['anki_ust:vic_engine_loop_duration'], bins='auto')
ax3.set_title("vic_engine_loop_duration")
plt.show()

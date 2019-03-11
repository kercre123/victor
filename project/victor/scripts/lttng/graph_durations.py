#!/usr/bin/env python3
import pickle
import argparse
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser(description='Graph vic-robot duration')
parser.add_argument('-f', '--file', help='Path of file to load trace from', metavar='f')
args = parser.parse_args()

with open(args.file, 'rb') as handle:
    trace = pickle.load(handle)

    anki_events = {'anki_ust:vic_robot_loop_duration': [],
                   'anki_ust:vic_anim_loop_duration': [],
                   'anki_ust:vic_engine_loop_duration': []}

    for k,v in trace.items():
        events = trace[k]
        for event in events:
            if event['name'] in anki_events:
                anki_events[event['name']].append(event['duration'])

    fix, (ax1, ax2,ax3) = plt.subplots(nrows=3,ncols=1, figsize=(4,8))
    ax1.hist(anki_events['anki_ust:vic_robot_loop_duration'], bins='auto')
    ax1.set_title("vic_robot_loop_duration")

    ax2.hist(anki_events['anki_ust:vic_anim_loop_duration'], bins='auto')
    ax2.set_title("vic_anim_loop_duration")

    ax3.hist(anki_events['anki_ust:vic_engine_loop_duration'], bins='auto')
    ax3.set_title("vic_engine_loop_duration")
    plt.show()

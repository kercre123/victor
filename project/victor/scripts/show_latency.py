from __future__ import print_function

import sys
import json

import numpy as np
import matplotlib.pyplot as plt


ANIM_FILE     = './anim_latency.json'
ENGINE_FILE   = './engine_latency.json'

ignored_steps = set([ 'Cloud.StoppedListening', 'VoiceRecording.Timeout' ])

interval_data = []
transition_data = {}

def sort_data(elem):
  return elem['time']

with open(ANIM_FILE, "r") as read_file:
  anim_data = json.load(read_file)
  for count, interval in enumerate(anim_data):
    all_steps = []
    for step in interval:
      if step['id'] not in ignored_steps:
        # differentiate the different processes by simply altering their thread id
        step['thread'] += 100
        all_steps.append( step )
    interval_data.append( all_steps )

with open(ENGINE_FILE, "r") as read_file:
  engine_data = json.load(read_file)

  if len(engine_data) != len(interval_data):
    print('ERROR: anim and engine data set sizes do not match')
    sys.exit()

  for count, interval in enumerate(engine_data):
    all_steps = []
    for step in interval:
      if step['id'] not in ignored_steps:
        all_steps.append( step )
    interval_data[count].extend( all_steps )

for interval in interval_data:
  interval.sort(key=sort_data)
  start_time = interval[0]['time']
  for index, step in enumerate(interval):
    step['time'] = step['time'] - start_time

    if index > 0:
      prevStep = interval[index-1]
      key = prevStep['id'] + ' -> ' + step['id']

      step_time = step['time'] - prevStep['time']
      step_ticks = step['tick'] - prevStep['tick']

      if step['thread'] != prevStep['thread']:
        step_ticks = -1

      if key in transition_data:
        transition = transition_data[key]
        transition['count'] += 1
        transition['time'] += step_time
        transition['tick'] += step_ticks
      else:
        transition = {}
        transition['level'] = index - 1
        transition['id'] = key
        transition['time'] = step_time
        transition['tick'] = step_ticks
        transition['count'] = 1
        transition_data[key] = transition

def transition_sort(elem):
  return elem['level']

sorted_transitions = sorted( transition_data.values(), key=transition_sort )

for transition in sorted_transitions:
  count = transition['count']
  print('[{}] time {}ms, ticks {}'.format(transition['id'],
                                          int(transition['time'] / count),
                                          int(transition['tick'] / count)))

def DrawChart():
  print('My Chart ...')

  labels = []
  time_vals = []
  tick_vals = []
  for transition in sorted_transitions[::-1]:
    labels.append( transition['id'] )

    count = transition['count']
    time_vals.append( int(transition['time'] / count) )
    tick_vals.append( int(transition['tick'] / count) )

  fig, ax = plt.subplots(figsize=(20, 10))
  ax2 = ax.twiny()

  y_pos = np.arange(len(labels))
  bar_height = 0.25
  opacity = 0.8

  rects1 = ax.barh(y_pos, time_vals, bar_height,
                    alpha=opacity,
                    color='b',
                    label='Time (ms)')

  rects2 = ax2.barh(y_pos + bar_height, tick_vals, bar_height,
                    alpha=opacity,
                    color='g',
                    label='Ticks')

  plt.yticks(y_pos, labels)
  plt.legend([rects1, rects2], ['Time (ms)', 'Ticks'])
  plt.title('Voice Command Latency')
  plt.tight_layout()
  plt.show()



DrawChart()

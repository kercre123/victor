#! /usr/bin/env python

# tool to quickly generate patterns for the Pattern Play demo of Cozmo
# Mark Pauley (10/14/2015)

import json
import collections

""" 
Schema:
{
  "Memory Bank 001": 
  [
    {
      "vertical": true,
      "facing_cozmo": true,
      "blocks": 
       [
        { 
          "front": "RGB", #front
          "back": "",     #back
          "left": "",     #left
          "right": ""     #right
        },
        {
          "front": "RGB", #front
          "back": "",     #back
          "left": "",     #left
          "right": ""     #right
        },
        {
          "front": "RGB", #front
          "back": "",     #back
          "left": "",     #left
          "right": ""     #right
        },
        {
          "front": "RGB", #front
          "back": "",     #back
          "left": "",     #left
          "right": ""     #right
        },
      ],
    },
    ...
  ],
  "Memory Bank 002": [
    ...
  ],
  ...
}
"""

PatternMemory = {}

side_names = ["front", "back", "left", "right"]

def generate_simple_lights(num_blocks, num_lights_on, vertical=False, facing_cozmo=False):
    result = []
    rotating_names = collections.deque(side_names)
    for _ in range(num_blocks + 1):
        cur_names = collections.deque(rotating_names)
        cur_dict = {}
        for _ in range(1, num_lights_on + 1):
            cur_dict[cur_names.popleft()] = "G"
        while(len(cur_names) > 0):
            cur_dict[cur_names.popleft()] = ""
        result.append({
            'vertical': vertical,
            'facing_cozmo': facing_cozmo,
            'blocks': [cur_dict for x in range(num_blocks)]
        })
        rotating_names.rotate(1)
    return result

# up-orientation
bank_num = 1
for i in range(1,4):
    PatternMemory['Memory Bank {0:03d}'.format(bank_num)] \
        = generate_simple_lights(3, i)
    bank_num = bank_num + 1

for i in range(1,4):
    PatternMemory['Memory Bank {0:03d}'.format(bank_num)] \
        = generate_simple_lights(3, i, facing_cozmo=False)
    bank_num = bank_num + 1

for i in range(1, 4):
    PatternMemory['Memory Bank {0:03d}'.format(bank_num)] \
        = generate_simple_lights(3, i, vertical=True)
    bank_num = bank_num + 1

for i in range(1, 4):
    PatternMemory['Memory Bank {0:03d}'.format(bank_num)] \
        = generate_simple_lights(3, i, vertical=True, facing_cozmo=True)
    bank_num = bank_num + 1

print json.dumps(PatternMemory, sort_keys=True, indent=2)



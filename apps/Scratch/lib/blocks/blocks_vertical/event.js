/**
 * @license
 * Visual Blocks Editor
 *
 * Copyright 2016 Massachusetts Institute of Technology
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

'use strict';

goog.provide('Blockly.Blocks.event');

goog.require('Blockly.Blocks');
goog.require('Blockly.Colours');
goog.require('Blockly.constants');
goog.require('Blockly.ScratchBlocks.VerticalExtensions');


Blockly.Blocks['event_whenflagclicked'] = {
  /**
   * Block for when flag clicked.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "event_whenflagclicked",
      "message0": "%{BKY_EVENTS_GREEN_FLAG_SCRATCH_2}",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "green-flag.svg",
          "width": 24,
          "height": 24,
          "alt": "flag"
        }
      ],
      "category": Blockly.Categories.event,
      "extensions": ["colours_event", "shape_hat"]
    });
  }
};

Blockly.Blocks['event_whenthisspriteclicked'] = {
  /**
   * Block for when this sprite clicked.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "when this sprite clicked",
      "category": Blockly.Categories.event,
      "extensions": ["colours_event", "shape_hat"]
    });
  }
};

Blockly.Blocks['event_whenbroadcastreceived'] = {
  /**
   * Block for when broadcast received.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "event_whenbroadcastreceived",
      "message0": "%{BKY_EVENTS_BROADCAST_HAT}",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "BROADCAST_OPTION",
          "options": [
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_1_SCRATCH_2}', 'message1'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_2_SCRATCH_2}', 'message2'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_3_SCRATCH_2}', 'message3'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_4_SCRATCH_2}', 'message4'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_5_SCRATCH_2}', 'message5'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_6_SCRATCH_2}', 'message6'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_7_SCRATCH_2}', 'message7'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_8_SCRATCH_2}', 'message8'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_9_SCRATCH_2}', 'message9'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_10_SCRATCH_2}', 'message10'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_11_SCRATCH_2}', 'message11'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_12_SCRATCH_2}', 'message12'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_13_SCRATCH_2}', 'message13'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_14_SCRATCH_2}', 'message14'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_15_SCRATCH_2}', 'message15'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_16_SCRATCH_2}', 'message16'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_17_SCRATCH_2}', 'message17'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_18_SCRATCH_2}', 'message18'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_19_SCRATCH_2}', 'message19'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_20_SCRATCH_2}', 'message20'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_21_SCRATCH_2}', 'message21'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_22_SCRATCH_2}', 'message22'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_23_SCRATCH_2}', 'message23'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_24_SCRATCH_2}', 'message24'],
            ['%{BKY_EVENTS_BROADCAST_MESSAGE_25_SCRATCH_2}', 'message25']
            /*['%{BKY_EVENTS_BROADCAST_NEW_MESSAGE_SCRATCH_2}', 'new message']*/
          ]
        }
      ],
      "category": Blockly.Categories.event,
      "extensions": ["colours_event", "shape_hat"]
    });
  }
};

Blockly.Blocks['event_whenbackdropswitchesto'] = {
  /**
   * Block for when the current backdrop switched to a selected backdrop.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "when backdrop switches to %1",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "BACKDROP",
          "options": [
              ['backdrop1', 'BACKDROP1']
          ]
        }
      ],
      "category": Blockly.Categories.event,
      "extensions": ["colours_event", "shape_hat"]
    });
  }
};

Blockly.Blocks['event_whengreaterthan'] = {
  /**
   * Block for when loudness/timer/video motion is greater than the value.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "when %1 > %2",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "WHENGREATERTHANMENU",
          "options": [
              ['timer', 'TIMER']
          ]
        },
        {
          "type": "input_value",
          "name": "VALUE"
        }
      ],
      "category": Blockly.Categories.event,
      "extensions": ["colours_event", "shape_hat"]
    });
  }
};

Blockly.Blocks['event_broadcast_menu'] = {
  /**
   * Broadcast drop-down menu.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "BROADCAST_OPTION",
            "options": [
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_1_SCRATCH_2}', 'message1'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_2_SCRATCH_2}', 'message2'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_3_SCRATCH_2}', 'message3'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_4_SCRATCH_2}', 'message4'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_5_SCRATCH_2}', 'message5'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_6_SCRATCH_2}', 'message6'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_7_SCRATCH_2}', 'message7'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_8_SCRATCH_2}', 'message8'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_9_SCRATCH_2}', 'message9'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_10_SCRATCH_2}', 'message10'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_11_SCRATCH_2}', 'message11'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_12_SCRATCH_2}', 'message12'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_13_SCRATCH_2}', 'message13'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_14_SCRATCH_2}', 'message14'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_15_SCRATCH_2}', 'message15'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_16_SCRATCH_2}', 'message16'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_17_SCRATCH_2}', 'message17'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_18_SCRATCH_2}', 'message18'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_19_SCRATCH_2}', 'message19'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_20_SCRATCH_2}', 'message20'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_21_SCRATCH_2}', 'message21'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_22_SCRATCH_2}', 'message22'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_23_SCRATCH_2}', 'message23'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_24_SCRATCH_2}', 'message24'],
              ['%{BKY_EVENTS_BROADCAST_MESSAGE_25_SCRATCH_2}', 'message25']
              /*['%{BKY_EVENTS_BROADCAST_NEW_MESSAGE_SCRATCH_2}', 'new message']*/
            ]
          }
        ],
        "colour": Blockly.Colours.event.secondary,
        "colourSecondary": Blockly.Colours.event.secondary,
        "colourTertiary": Blockly.Colours.event.tertiary,
        "extensions": ["output_string"]
      });
  }
};

Blockly.Blocks['event_broadcast'] = {
  /**
   * Block to send a broadcast.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "event_broadcast",
      "message0": "%{BKY_EVENTS_BROADCAST_SCRATCH_2}",
      "args0": [
        {
          "type": "input_value",
          "name": "BROADCAST_OPTION"
        }
      ],
      "category": Blockly.Categories.event,
      "extensions": ["colours_event", "shape_statement"]
    });
  }
};

Blockly.Blocks['event_broadcastandwait'] = {
  /**
   * Block to send a broadcast.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_EVENTS_BROADCAST_AND_WAIT}",
      "args0": [
        {
          "type": "input_value",
          "name": "BROADCAST_OPTION"
        }
      ],
      "category": Blockly.Categories.event,
      "extensions": ["colours_event", "shape_statement"]
    });
  }
};

Blockly.Blocks['event_whenkeypressed'] = {
  /**
   * Block to send a broadcast.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "event_whenkeypressed",
      "message0": "when %1 key pressed",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "KEY_OPTION",
          "options": [
            ['space', 'space'],
            ['left arrow', 'left arrow'],
            ['right arrow', 'right arrow'],
            ['down arrow', 'down arrow'],
            ['up arrow', 'up arrow'],
            ['any', 'any'],
            ['a', 'a'],
            ['b', 'b'],
            ['c', 'c'],
            ['d', 'd'],
            ['e', 'e'],
            ['f', 'f'],
            ['g', 'g'],
            ['h', 'h'],
            ['i', 'i'],
            ['j', 'j'],
            ['k', 'k'],
            ['l', 'l'],
            ['m', 'm'],
            ['n', 'n'],
            ['o', 'o'],
            ['p', 'p'],
            ['q', 'q'],
            ['r', 'r'],
            ['s', 's'],
            ['t', 't'],
            ['u', 'u'],
            ['v', 'v'],
            ['w', 'w'],
            ['x', 'x'],
            ['y', 'y'],
            ['z', 'z'],
            ['0', '0'],
            ['1', '1'],
            ['2', '2'],
            ['3', '3'],
            ['4', '4'],
            ['5', '5'],
            ['6', '6'],
            ['7', '7'],
            ['8', '8'],
            ['9', '9']
          ]
        }
      ],
      "category": Blockly.Categories.event,
      "extensions": ["colours_event", "shape_hat"]
    });
  }
};

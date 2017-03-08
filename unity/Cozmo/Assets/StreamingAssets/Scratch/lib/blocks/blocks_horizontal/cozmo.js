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

/**
 * @fileoverview Wedo blocks for Scratch (Horizontal)
 * @author ascii@media.mit.edu <Andrew Sliwinski>
 */
'use strict';

goog.provide('Blockly.Blocks.cozmo');

goog.require('Blockly.Blocks');

goog.require('Blockly.Colours');

Blockly.Blocks['dropdown_cozmo_setcolor'] = {
  /**
   * Block for set color drop-down (used for shadow).
   * @this Blockly.Block
   */
  init: function() {
    this.appendDummyInput()
        .appendField(new Blockly.FieldIconMenu([
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_mystery.svg',
              value: 'mystery', width: 48, height: 48, alt: 'Mystery'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_yellow.svg',
              value: 'yellow', width: 48, height: 48, alt: 'Yellow'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_orange.svg',
            value: 'orange', width: 48, height: 48, alt: 'Orange'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_coral.svg',
            value: 'coral', width: 48, height: 48, alt: 'Coral'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_purple.svg',
            value: 'purple', width: 48, height: 48, alt: 'Purple'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_blue.svg',
            value: 'blue', width: 48, height: 48, alt: 'Blue'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_green.svg',
            value: 'green', width: 48, height: 48, alt: 'Green'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_white.svg',
              value: 'white', width: 48, height: 48, alt: 'White'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_black.svg',
              value: 'off', width: 48, height: 48, alt: 'Off'}
        ]), 'CHOICE');
    this.setOutput(true);
    this.setColour(Blockly.Colours.looks.primary,
      Blockly.Colours.looks.secondary,
      Blockly.Colours.looks.tertiary
    );
  }
};

Blockly.Blocks['cozmo_setbackpackcolor'] = {
  /**
   * Block to set color of LED
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_setbackpackcolor",
      "message0": "%1 %2",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/set-led_blue.svg",
          "width": 40,
          "height": 40,
          "alt": "Set LED Color"
        },
        {
          "type": "input_value",
          "name": "CHOICE"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.looks,
      "colour": Blockly.Colours.looks.primary,
      "colourSecondary": Blockly.Colours.looks.secondary,
      "colourTertiary": Blockly.Colours.looks.tertiary
    });
  }
};

Blockly.Blocks['cozmo_drive_forward'] = {
  /**
   * Block to drive forward.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_drive_forward",
      "message0": "%1 %2",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-forward.svg",
          "width": 40,
          "height": 40,
          "alt": "Drive forward"
        },
        {
          "type": "input_value",
          "name": "DISTANCE",
          "check": "Number"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.motion,
      "colour": Blockly.Colours.motion.primary,
      "colourSecondary": Blockly.Colours.motion.secondary,
      "colourTertiary": Blockly.Colours.motion.tertiary
    });
  }
};

Blockly.Blocks['cozmo_drive_backward'] = {
  /**
   * Block to drive backward.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_drive_backward",
      "message0": "%1 %2",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-backward.svg",
          "width": 40,
          "height": 40,
          "alt": "Drive backward"
        },
        {
          "type": "input_value",
          "name": "DISTANCE",
          "check": "Number"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.motion,
      "colour": Blockly.Colours.motion.primary,
      "colourSecondary": Blockly.Colours.motion.secondary,
      "colourTertiary": Blockly.Colours.motion.tertiary
    });
  }
};


Blockly.Blocks['dropdown_cozmo_setanimation'] = {
  /**
   * Block for set animation drop-down.
   * @this Blockly.Block
   */
  init: function() {
    this.appendDummyInput()
        .appendField(new Blockly.FieldIconMenu([
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-happy.svg',
              value: 'happy', width: 48, height: 48, alt: 'Happy'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-victory.svg',
            value: 'victory', width: 48, height: 48, alt: 'Victory'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-unhappy.svg',
            value: 'unhappy', width: 48, height: 48, alt: 'Unhappy'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-surprise.svg',
            value: 'surprise', width: 48, height: 48, alt: 'Surprise'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-dog.svg',
            value: 'dog', width: 48, height: 48, alt: 'Dog'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-cat.svg',
            value: 'cat', width: 48, height: 48, alt: 'Cat'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-sneeze.svg',
            value: 'sneeze', width: 48, height: 48, alt: 'Sneeze'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-excited.svg',
              value: 'excited', width: 48, height: 48, alt: 'Excited'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-thinking.svg',
              value: 'thinking', width: 48, height: 48, alt: 'Thinking'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-bored.svg',
            value: 'bored', width: 48, height: 48, alt: 'Bored'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-frustrated.svg',
            value: 'frustrated', width: 48, height: 48, alt: 'Frustrated'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-chatty.svg',
            value: 'chatty', width: 48, height: 48, alt: 'Chatty'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-dejected.svg',
            value: 'dejected', width: 48, height: 48, alt: 'Dejected'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-sleep.svg',
              value: 'sleep', width: 48, height: 48, alt: 'Sleep'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-anim-mystery.svg',
              value: 'mystery', width: 48, height: 48, alt: 'Mystery'}
        ]), 'CHOICE');
    this.setOutput(true);
    this.setColour(Blockly.Colours.looks.primary,
      Blockly.Colours.looks.secondary,
      Blockly.Colours.looks.tertiary
    );
  }
};

Blockly.Blocks['cozmo_animation'] = {
  /**
   * Block to play an animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_animation",
      "message0": "%1 %2",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-happy.svg",
          "width": 40,
          "height": 40,
          "alt": "Play an animation"
        },
        {
          "type": "input_value",
          "name": "CHOICE"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.looks,
      "colour": Blockly.Colours.looks.primary,
      "colourSecondary": Blockly.Colours.looks.secondary,
      "colourTertiary": Blockly.Colours.looks.tertiary
    });
  }
};

Blockly.Blocks['dropdown_cozmo_liftheight'] = {
  /**
   * Block for Cozmo forklift drop-down (used for shadow).
   * @this Blockly.Block
   */
  init: function() {
    this.appendDummyInput()
        .appendField(new Blockly.FieldIconMenu([
            {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-forklift-low.svg',
              value: 'low', width: 48, height: 48, alt: 'Low'},
            {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-forklift-medium.svg',
              value: 'medium', width: 48, height: 48, alt: 'Medium'},
            {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-forklift-high.svg',
              value: 'high', width: 48, height: 48, alt: 'High'}
        ]), 'CHOICE');
    this.setOutput(true);
    this.setColour(Blockly.Colours.motion.primary,
      Blockly.Colours.motion.secondary,
      Blockly.Colours.motion.tertiary
    );
  }
};

Blockly.Blocks['cozmo_liftheight'] = {
  /**
   * Block to set the lift height.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_liftheight",
      "message0": "%1 %2",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-forklift-medium.svg",
          "width": 40,
          "height": 40,
          "alt": "Move lift"
        },
        {
          "type": "input_value",
          "name": "CHOICE"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.motion,
      "colour": Blockly.Colours.motion.primary,
      "colourSecondary": Blockly.Colours.motion.secondary,
      "colourTertiary": Blockly.Colours.motion.tertiary
    });
  }
};

Blockly.Blocks['dropdown_cozmo_headangle'] = {
  /**
   * Block for Cozmo head angle drop-down.
   * @this Blockly.Block
   */
  init: function() {
    this.appendDummyInput()
        .appendField(new Blockly.FieldIconMenu([
            {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-head-angle-low.svg',
              value: 'low', width: 48, height: 48, alt: 'Low'},
            {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-head-angle-medium.svg',
              value: 'medium', width: 48, height: 48, alt: 'Medium'},
            {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-head-angle-high.svg',
              value: 'high', width: 48, height: 48, alt: 'High'}
        ]), 'CHOICE');
    this.setOutput(true);
    this.setColour(Blockly.Colours.motion.primary,
      Blockly.Colours.motion.secondary,
      Blockly.Colours.motion.tertiary
    );
  }
};

Blockly.Blocks['cozmo_headangle'] = {
  /**
   * Block to set Cozmo's head angle.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_headangle",
      "message0": "%1 %2",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-head-angle-high.svg",
          "width": 40,
          "height": 40,
          "alt": "Move head"
        },
        {
          "type": "input_value",
          "name": "CHOICE"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.motion,
      "colour": Blockly.Colours.motion.primary,
      "colourSecondary": Blockly.Colours.motion.secondary,
      "colourTertiary": Blockly.Colours.motion.tertiary
    });
  }
};

Blockly.Blocks['cozmo_dock_with_cube'] = {
  /**
   * Block to tell Cozmo to dock with a cube that he can see, if any.
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
      "id": "cozmo_dock_with_cube",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-dock-with-cube.svg",
          "width": 40,
          "height": 40,
          "alt": "Dock with cube"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.motion,
      "colour": Blockly.Colours.motion.primary,
      "colourSecondary": Blockly.Colours.motion.secondary,
      "colourTertiary": Blockly.Colours.motion.tertiary
    });
  }
};

Blockly.Blocks['cozmo_says'] = {
  /**
    * Block to make Cozmo speak text
    * @this Blockly.Block
  */
init: function() {
  this.jsonInit({
    "id": "cozmo_says",
    "message0": "%1 %2",
    "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo_says.svg",
          "width": 40,
          "height": 40,
          "alt": "Make robot speak"
        },
        {
          "type": "input_value",
          "name": "STRING",
          "check": "String"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.looks,
      "colour": Blockly.Colours.looks.primary,
      "colourSecondary": Blockly.Colours.looks.secondary,
      "colourTertiary": Blockly.Colours.looks.tertiary
    });
  }
};

Blockly.Blocks['dropdown_cozmo_drive_speed'] = {
  /**
   * Block to set speed for Cozmo drive forward and backward.
   * @this Blockly.Block
   */
  init: function() {
    this.appendDummyInput()
        .appendField(new Blockly.FieldIconMenu([
            {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-drive-slow.svg',
              value: 'slow', width: 48, height: 48, alt: 'Slow'},
            {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-drive-medium.svg',
              value: 'medium', width: 48, height: 48, alt: 'Medium'},
            {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/cozmo-drive-fast.svg',
              value: 'fast', width: 48, height: 48, alt: 'Fast'}
        ]), 'CHOICE');
    this.setOutput(true);
    this.setColour(Blockly.Colours.control.primary,
      Blockly.Colours.control.secondary,
      Blockly.Colours.control.tertiary
    );
  }
};

Blockly.Blocks['cozmo_drive_speed'] = {
  /**
   * Block to make Cozmo drive forward or backward faster.
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
      "id": "cozmo__drive_speed",
      "message0": "%1 %2",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-drive-slow.svg",
          "width": 40,
          "height": 40,
          "alt": "Set drive speed"
        },
        {
          "type": "input_value",
          "name": "CHOICE"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.control,
      "colour": Blockly.Colours.control.primary,
      "colourSecondary": Blockly.Colours.control.secondary,
      "colourTertiary": Blockly.Colours.control.tertiary
    });
  }
};

Blockly.Blocks['cozmo_turn_left'] = {
  /**
   * Block to turn Cozmo 90 degrees left.
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
      "id": "cozmo_turn_left",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-turn-left.svg",
          "width": 40,
          "height": 40,
          "alt": "Turn left"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.motion,
      "colour": Blockly.Colours.motion.primary,
      "colourSecondary": Blockly.Colours.motion.secondary,
      "colourTertiary": Blockly.Colours.motion.tertiary
    });
  }
};

Blockly.Blocks['cozmo_turn_right'] = {
  /**
   * Block to turn Cozmo 90 degrees right.
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
      "id": "cozmo_turn_right",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-turn-right.svg",
          "width": 40,
          "height": 40,
          "alt": "Turn right"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.motion,
      "colour": Blockly.Colours.motion.primary,
      "colourSecondary": Blockly.Colours.motion.secondary,
      "colourTertiary": Blockly.Colours.motion.tertiary
    });
  }
};

Blockly.Blocks['cozmo_wait_for_face'] = {
  /**
   * Block to wait until a Cozmo sees a face.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-face.svg",
          "width": 40,
          "height": 40,
          "alt": "Wait until Cozmo sees face"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.control,
      "colour": Blockly.Colours.control.primary,
      "colourSecondary": Blockly.Colours.control.secondary,
      "colourTertiary": Blockly.Colours.control.tertiary
    });
  }
};

Blockly.Blocks['cozmo_wait_for_cube'] = {
  /**
   * Block to wait until a Cozmo sees a cube.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-cube.svg",
          "width": 40,
          "height": 40,
          "alt": "Wait until Cozmo sees a cube"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.control,
      "colour": Blockly.Colours.control.primary,
      "colourSecondary": Blockly.Colours.control.secondary,
      "colourTertiary": Blockly.Colours.control.tertiary
    });
  }
};

Blockly.Blocks['cozmo_wait_for_cube_tap'] = {
  /**
   * Block to wait until a cube that Cozmo can see is tapped.
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-cube-tap.svg",
          "width": 40,
          "height": 40,
          "alt": "Wait for cube tap"
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.control,
      "colour": Blockly.Colours.control.primary,
      "colourSecondary": Blockly.Colours.control.secondary,
      "colourTertiary": Blockly.Colours.control.tertiary
    });
  }
};

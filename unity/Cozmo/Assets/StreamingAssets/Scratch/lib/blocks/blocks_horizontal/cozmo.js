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
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_coral.svg',
            value: 'coral', width: 48, height: 48, alt: 'Coral'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_orange.svg',
            value: 'orange', width: 48, height: 48, alt: 'Orange'},            
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_yellow.svg',
              value: 'yellow', width: 48, height: 48, alt: 'Yellow'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_green.svg',
            value: 'green', width: 48, height: 48, alt: 'Green'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_blue.svg',
            value: 'blue', width: 48, height: 48, alt: 'Blue'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_purple.svg',
            value: 'purple', width: 48, height: 48, alt: 'Purple'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_white.svg',
              value: 'white', width: 48, height: 48, alt: 'White'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_black.svg',
              value: 'off', width: 48, height: 48, alt: 'Off'},
          {src: Blockly.mainWorkspace.options.pathToMedia + 'icons/set-led_mystery.svg',
              value: 'mystery', width: 48, height: 48, alt: 'Mystery'}
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
          "height": 40
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
    this.setTooltip("Light Backpack");
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
          "height": 40
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
    this.setTooltip("Drive Forward");
  }
};

Blockly.Blocks['cozmo_drive_forward_fast'] = {
  /**
   * Block to drive forward fast.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_drive_forward_fast",
      "message0": "%1 %2",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-forward-fast.svg",
          "width": 40,
          "height": 40
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
    this.setTooltip("Drive Forward Fast");
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
          "height": 40
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
    this.setTooltip("Drive Backward");
  }
};

Blockly.Blocks['cozmo_drive_backward_fast'] = {
  /**
   * Block to drive backward fast.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_drive_backward_fast",
      "message0": "%1 %2",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-backward-fast.svg",
          "width": 40,
          "height": 40
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
    this.setTooltip("Drive Backward Fast");
  }
};

Blockly.Blocks['cozmo_happy_animation'] = {
  /**
   * Block to play a happy animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_happy_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-happy.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Happy");
  }
};

Blockly.Blocks['cozmo_victory_animation'] = {
  /**
   * Block to play a victory animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_victory_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-victory.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Like Winner");
  }
};

Blockly.Blocks['cozmo_unhappy_animation'] = {
  /**
   * Block to play a unhappy animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_unhappy_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-unhappy.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Sad");
  }
};

Blockly.Blocks['cozmo_surprise_animation'] = {
  /**
   * Block to play a surprise animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_surprise_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-surprise.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Surprised");
  }
};

Blockly.Blocks['cozmo_dog_animation'] = {
  /**
   * Block to play a "see dog" animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_dog_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-dog.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Like Dog");
  }
};

Blockly.Blocks['cozmo_cat_animation'] = {
  /**
   * Block to play a "see cat" animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_cat_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-cat.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Like Cat");
  }
};

Blockly.Blocks['cozmo_sneeze_animation'] = {
  /**
   * Block to play a sneeze animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_sneeze_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-sneeze.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Sneeze");
  }
};

Blockly.Blocks['cozmo_excited_animation'] = {
  /**
   * Block to play an excited animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_excited_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-excited.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Excited");
  }
};

Blockly.Blocks['cozmo_thinking_animation'] = {
  /**
   * Block to play a thinking animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_thinking_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-thinking.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Think Hard");
  }
};

Blockly.Blocks['cozmo_bored_animation'] = {
  /**
   * Block to play a bored animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_bored_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-bored.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Bored");
  }
};

Blockly.Blocks['cozmo_frustrated_animation'] = {
  /**
   * Block to play a frustrated animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_frustrated_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-frustrated.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Frustrated");
  }
};

Blockly.Blocks['cozmo_chatty_animation'] = {
  /**
   * Block to play a chatty animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_chatty_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-chatty.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Chatty");
  }
};

Blockly.Blocks['cozmo_dejected_animation'] = {
  /**
   * Block to play a disappointed animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_dejected_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-dejected.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Act Disappointed");
  }
};

Blockly.Blocks['cozmo_sleep_animation'] = {
  /**
   * Block to play a sleep animation.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_sleep_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-sleep.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Snore");
  }
};

Blockly.Blocks['cozmo_mystery_animation'] = {
  /**
   * Block to play a mystery animation (one will be chosen at random).
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_mystery_animation",
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-anim-mystery.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.actions,
      "colour": Blockly.Colours.actions.primary,
      "colourSecondary": Blockly.Colours.actions.secondary,
      "colourTertiary": Blockly.Colours.actions.tertiary
    });
    this.setTooltip("Mystery Animation");
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
    this.setColour(Blockly.Colours.looks.primary,
      Blockly.Colours.looks.secondary,
      Blockly.Colours.looks.tertiary
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
          "height": 40
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
    this.setTooltip("Move Lift");
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
    this.setColour(Blockly.Colours.looks.primary,
      Blockly.Colours.looks.secondary,
      Blockly.Colours.looks.tertiary
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
          "height": 40
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
    this.setTooltip("Move Head");
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
          "height": 40
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
    this.setTooltip("Drive to Cube");
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
          "height": 40
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
    this.setTooltip("Say");
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
          "height": 40
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
    this.setTooltip("Turn Left");
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
          "height": 40
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
    this.setTooltip("Turn Right");
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
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
    this.setTooltip("Wait to See Face");
  }
};

Blockly.Blocks['cozmo_wait_for_happy_face'] = {
  /**
   * Block to wait until a Cozmo sees a happy face.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-face-happy.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
    this.setTooltip("Wait to See Smile");
  }
};

Blockly.Blocks['cozmo_wait_for_sad_face'] = {
  /**
   * Block to wait until a Cozmo sees a sad face.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%1",
      "args0": [
        {
          "type": "field_image",
          "src": Blockly.mainWorkspace.options.pathToMedia + "icons/cozmo-face-sad.svg",
          "width": 40,
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
    this.setTooltip("Wait to See Frown");
  }
};

Blockly.Blocks['cozmo_wait_until_see_cube'] = {
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
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
    this.setTooltip("Wait to See Cube");
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
          "height": 40
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
    this.setTooltip("Wait for Cube Tap");
  }
};

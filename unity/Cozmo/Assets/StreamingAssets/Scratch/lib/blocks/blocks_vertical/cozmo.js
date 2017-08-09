/**
 * @fileoverview Cozmo blocks for Scratch (Vertical)
 */
'use strict';

goog.provide('Blockly.Blocks.cozmo');

goog.require('Blockly.Blocks');

goog.require('Blockly.Colours');

// ========================================================================================================================
// Horizontal block clones, calling the same methods, just with text instead of image icons
// ========================================================================================================================

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
      "message0": "Set Backpack Color %1",
      "args0": [
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

Blockly.Blocks['cozmo_vert_setbackpackcolor'] = {
  /**
   * Block to set color of LED
   * @this Blockly.Block
   */

  init: function() {
    this.jsonInit({
      "id": "cozmo_vert_setbackpackcolor",
      "message0": "set backpack color to %1",
      "args0": [
        {
          "type": "input_value",
          "name": "COLOR"
        }
      ],
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.looks,
      "colour": Blockly.Colours.looks.primary,
      "colourSecondary": Blockly.Colours.looks.secondary,
      "colourTertiary": Blockly.Colours.looks.tertiary
    });
  }
};

Blockly.Blocks['cozmo_play_animation_from_dropdown'] = {
  /**
    * Block to make Cozmo play an animation
    * @this Blockly.Block
  */
init: function() {
  this.jsonInit({
    "id": "cozmo_play_animation_from_dropdown",
    "message0": "Play %1 Animation with %2, %3, %4",
    "args0": [
        {
          "type": "field_dropdown",
          "name": "ANIMATION",
          "options": [
              ['Happy', 'happy'],
              ['Winner', 'victory'],
              ['Sad', 'unhappy'],
              ['Surprised', 'surprise'],
              ['Dog', 'dog'],
              ['Cat', 'cat'],
              ['Sneeze', 'sneeze'],
              ['Excited', 'excited'],
              ['Think Hard', 'thinking'],
              ['Bored', 'bored'],
              ['Frustrated', 'frustrated'],
              ['Chatty', 'chatty'],
              ['Disappointed', 'dejected'],
              ['Snore', 'sleep'],
              ['Mystery', 'mystery']      
            ]
        },
        {
          "type": "field_dropdown",
          "name": "IGNORE_WHEELS",
          "options": [
              ['Wheels', 'false'],
              ['No Wheels', 'true'],    
            ]
        },
        {
          "type": "field_dropdown",
          "name": "IGNORE_HEAD",
          "options": [
              ['Head', 'false'],
              ['No Head', 'true'],    
            ]
        },
        {
          "type": "field_dropdown",
          "name": "IGNORE_LIFT",
          "options": [
              ['Lift', 'false'],
              ['No Lift', 'true'],    
            ]
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
      "message0": "dock with cube",
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

Blockly.Blocks['cozmo_vert_dock_with_cube_by_id'] = {
  /**
   * Block to tell Cozmo to dock with a cube that he can see, if any.
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
      "id": "cozmo_vert_dock_with_cube_by_id",
      "message0": "Dock with cube %1",
      "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
            ]
          },
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
    "message0": "Say %1",
    "args0": [
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

Blockly.Blocks['cozmo_event_on_face'] = {
  /**
   * Block to wait until a Cozmo sees a face.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_event_on_face",
      "message0": "when see face",
      "inputsInline": true,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
  }
};

Blockly.Blocks['cozmo_event_on_happy_face'] = {
  /**
   * Block to wait until a Cozmo sees a happy face.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_event_on_happy_face",
      "message0": "when see happy face",
      "inputsInline": true,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
  }
};

Blockly.Blocks['cozmo_event_on_sad_face'] = {
  /**
   * Block to wait until a Cozmo sees a sad face.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_event_on_sad_face",
      "message0": "when see sad face",
      "inputsInline": true,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
  }
};

Blockly.Blocks['cozmo_event_on_see_cube'] = {
  /**
   * Block to wait until a Cozmo sees a cube.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_event_on_see_cube",
      "message0": "when see cube",
      "inputsInline": true,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
  }
};

Blockly.Blocks['cozmo_event_on_cube_tap'] = {
  /**
   * Block to detect that a specific cube is tapped.
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
      "id": "cozmo_event_on_cube_tap",
      "message0": "when cube %1 is tapped",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "CUBE_SELECT",
          "options": [
            ['1', '1'],
            ['2', '2'],
            ['3', '3']
          ]
        },
      ],
      "inputsInline": true,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
  }
};

// ========================================================================================================================
// Vertical specific blocks
// ========================================================================================================================

// Sensors / Inputs

// Cozmo

Blockly.Blocks['cozmo_vert_get_position_3d'] = {
  /**
   * Block to read the X,Y or Z component of Cozmo's position
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "Cozmo position %1",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "AXIS",
            "options": [
              ['x', '0'],
              ['y', '1'],
              ['z', '2']
            ]
          }
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.sensing.primary,
        "colourSecondary": Blockly.Colours.sensing.secondary,
        "colourTertiary": Blockly.Colours.sensing.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
      });
  }
};

Blockly.Blocks['cozmo_vert_get_pitch'] = {
  /**
   * Block to read Cozmo's pitch (angle about y-axis)
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_vert_get_pitch",
      "message0": "Cozmo pitch",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Number",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_get_roll'] = {
  /**
   * Block to read Cozmo's roll (angle about x-axis)
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_vert_get_roll",
      "message0": "Cozmo roll",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Number",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_get_yaw'] = {
  /**
   * Block to read Cozmo's yaw (angle about z-axis)
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_vert_get_yaw",
      "message0": "Cozmo yaw",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Number",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_get_lift_height'] = {
  /**
   * Block to read Cozmo's lift height (as a factor from 0.0 to 1.0)
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Cozmo lift height",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Number",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_get_head_angle'] = {
  /**
   * Block to read Cozmo's lift height (as a factor from 0.0 to 1.0)
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Cozmo head angle",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Number",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

// Faces (Sensors / Inputs)

Blockly.Blocks['cozmo_vert_face_get_is_visible'] = {
  /**
   * Block to read if there is a visible most recently seen face
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Face is visible",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Boolean",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_face_get_name'] = {
  /**
   * Block to read the name of the most recently seen face
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Face name",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "String",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_face_get_position_2d'] = {
  /**
   * Block to read the camera x,y position of the most recently seen face
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Face Camera %1",
      "args0": [
          {
            "type": "field_dropdown",
            "name": "AXIS",
            "options": [
              ['x', '0'],
              ['y', '1']
            ]
          }
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.sensing.primary,
        "colourSecondary": Blockly.Colours.sensing.secondary,
        "colourTertiary": Blockly.Colours.sensing.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_face_get_position_3d'] = {
  /**
   * Block to read the world x,y,z position of the most recently seen face
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Face Position %1",
      "args0": [
          {
            "type": "field_dropdown",
            "name": "AXIS",
            "options": [
              ['x', '0'],
              ['y', '1'],
              ['z', '2']
            ]
          }
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.sensing.primary,
        "colourSecondary": Blockly.Colours.sensing.secondary,
        "colourTertiary": Blockly.Colours.sensing.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

// Cubes (Sensors / Inputs)

Blockly.Blocks['cozmo_vert_cube_get_is_visible'] = {
  /**
   * Block to read if a given cube is visible
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Cube %1 is visible",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "CUBE_SELECT",
          "options": [
            ['1', '1'],
            ['2', '2'],
            ['3', '3']
          ]
        },
      ],
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Boolean",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_cube_get_position_2d'] = {
  /**
   * Block to read the camera x,y position of one of Cozmo's cube's
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      { 
        "message0": "Cube %1 Camera %2",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
            ]
          },
          {
            "type": "field_dropdown",
            "name": "AXIS",
            "options": [
              ['x', '0'],
              ['y', '1']
            ]
          }
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.sensing.primary,
        "colourSecondary": Blockly.Colours.sensing.secondary,
        "colourTertiary": Blockly.Colours.sensing.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
      });
  }
};

Blockly.Blocks['cozmo_vert_cube_get_position_3d'] = {
  /**
   * Block to read the X,Y or Z component of one of Cozmo's cube's position
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "Cube %1 position %2",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
            ]
          },
          {
            "type": "field_dropdown",
            "name": "AXIS",
            "options": [
              ['x', '0'],
              ['y', '1'],
              ['z', '2']
            ]
          }
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.sensing.primary,
        "colourSecondary": Blockly.Colours.sensing.secondary,
        "colourTertiary": Blockly.Colours.sensing.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
      });
  }
};

Blockly.Blocks['cozmo_vert_cube_get_pitch'] = {
  /**
   * Block to read the pitch of one of Cozmo's cube's position
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "Cube %1 pitch",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
            ]
          },
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.sensing.primary,
        "colourSecondary": Blockly.Colours.sensing.secondary,
        "colourTertiary": Blockly.Colours.sensing.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
      });
  }
};

Blockly.Blocks['cozmo_vert_cube_get_roll'] = {
  /**
   * Block to read the roll of one of Cozmo's cube's position
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "Cube %1 roll",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
            ]
          },
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.sensing.primary,
        "colourSecondary": Blockly.Colours.sensing.secondary,
        "colourTertiary": Blockly.Colours.sensing.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
      });
  }
};

Blockly.Blocks['cozmo_vert_cube_get_yaw'] = {
  /**
   * Block to read the yaw of one of Cozmo's cube's position
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "Cube %1 yaw",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
            ]
          },
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.sensing.primary,
        "colourSecondary": Blockly.Colours.sensing.secondary,
        "colourTertiary": Blockly.Colours.sensing.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
      });
  }
};

Blockly.Blocks['cozmo_vert_device_get_pitch'] = {
  /**
   * Block to read the roll of the user's device
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Device pitch",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Number",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_device_get_roll'] = {
  /**
   * Block to read the roll of the user's device
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Device roll",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Number",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_device_get_yaw'] = {
  /**
   * Block to read the roll of the user's device
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Device yaw",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Number",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

// Actions

Blockly.Blocks['cozmo_vert_drive'] = {
  /**
   * Block to drive Cozmo X mm forwards/backwards at the given speed
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_drive",
        "message0": "drive %1 at speed %2",
        "args0": [
          {
            "type": "input_value",
            "name": "DISTANCE"
          },
          {
            "type": "input_value",
            "name": "SPEED"
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

Blockly.Blocks['cozmo_vert_path_offset'] = {
  /**
   * Block to drive Cozmo to a given pose, offset from Cozmo's current pose
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_path_offset",
        "message0": "path forward:%1 left:%2 turn:%3",
        "args0": [
          {
            "type": "input_value",
            "name": "OFFSET_X"
          },
          {
            "type": "input_value",
            "name": "OFFSET_Y"
          },
          {
            "type": "input_value",
            "name": "OFFSET_ANGLE"
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

Blockly.Blocks['cozmo_vert_path_to'] = {
  /**
   * Block to drive Cozmo to a given pose
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_path_to",
        "message0": "path to x:%1 y:%2 angle:%3",
        "args0": [
          {
            "type": "input_value",
            "name": "NEW_X"
          },
          {
            "type": "input_value",
            "name": "NEW_Y"
          },
          {
            "type": "input_value",
            "name": "NEW_ANGLE"
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

Blockly.Blocks['cozmo_vert_turn'] = {
  /**
   * Block to turn Cozmo X degrees left or right (+ve left, -ve right).
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_turn",
        "message0": "turn %1 at speed %2",
        "args0": [
          {
            "type": "input_value",
            "name": "ANGLE"
          },
          {
            "type": "input_value",
            "name": "SPEED"
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

Blockly.Blocks['cozmo_vert_set_liftheight'] = {
  /**
   * Block to set the lift to the given height at the given speed
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_set_liftheight",
        "message0": "move lift to %1 at speed %2",
        "args0": [
          {
            "type": "input_value",
            "name": "HEIGHT_RATIO"
          },
          {
            "type": "input_value",
            "name": "SPEED"
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


Blockly.Blocks['cozmo_vert_set_headangle'] = {
  /**
   * Block to set the head to the given angle at the given speed
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_set_headangle", 
        "message0": "move head to %1 at speed %2",
        "args0": [
          {
            "type": "input_value",
            "name": "HEAD_ANGLE"
          },
          {
            "type": "input_value",
            "name": "SPEED"
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

Blockly.Blocks['cozmo_vert_set_cube_light_corners'] = {
  /**
   * Block to set a cube's corner lights individually
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "Set Cube %1 lights %2 %3 %4 %5",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "CUBE_SELECT",
          "options": [
            ['1', '1'],
            ['2', '2'],
            ['3', '3']
          ]
        },
        {
          "type": "field_dropdown",
          "name": "CORNER_1_COLOR",
          "options": [
            ['Coral', 'coral'],
            ['Orange', 'orange'],
            ['Yellow', 'yellow'],
            ['Green', 'green'],
            ['Blue', 'blue'],
            ['Purple', 'purple'],
            ['White', 'white'],
            ['Off', 'off'],
            ['Mystery', 'mystery']
          ]
        },
        {
          "type": "field_dropdown",
          "name": "CORNER_2_COLOR",
          "options": [
            ['Coral', 'coral'],
            ['Orange', 'orange'],
            ['Yellow', 'yellow'],
            ['Green', 'green'],
            ['Blue', 'blue'],
            ['Purple', 'purple'],
            ['White', 'white'],
            ['Off', 'off'],
            ['Mystery', 'mystery']
          ]
        },
        {
          "type": "field_dropdown",
          "name": "CORNER_3_COLOR",
          "options": [
            ['Coral', 'coral'],
            ['Orange', 'orange'],
            ['Yellow', 'yellow'],
            ['Green', 'green'],
            ['Blue', 'blue'],
            ['Purple', 'purple'],
            ['White', 'white'],
            ['Off', 'off'],
            ['Mystery', 'mystery']
          ]
        },
        {
          "type": "field_dropdown",
          "name": "CORNER_4_COLOR",
          "options": [
            ['Coral', 'coral'],
            ['Orange', 'orange'],
            ['Yellow', 'yellow'],
            ['Green', 'green'],
            ['Blue', 'blue'],
            ['Purple', 'purple'],
            ['White', 'white'],
            ['Off', 'off'],
            ['Mystery', 'mystery']
          ]
        }
      ],
      "inputsInline": true,
      "previousStatement": null,
      "nextStatement": null,
      "category": Blockly.Categories.looks,
      "colour": Blockly.Colours.looks.primary,
      "colourSecondary": Blockly.Colours.looks.secondary,
      "colourTertiary": Blockly.Colours.looks.tertiary,
    });
  }
};


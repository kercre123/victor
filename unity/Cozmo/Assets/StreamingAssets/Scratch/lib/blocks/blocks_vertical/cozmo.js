/**
 * @fileoverview Cozmo blocks for Scratch (Vertical)
 */
'use strict';

goog.provide('Blockly.Blocks.cozmo');

goog.require('Blockly.Blocks');

goog.require('Blockly.Colours');

Blockly.Blocks['cozmo_vert_setbackpackcolor'] = {
  /**
   * Block to set color of LED
   * @this Blockly.Block
   */

  init: function() {
    this.jsonInit({
      "id": "cozmo_vert_setbackpackcolor",
      "message0": "%{BKY_COZMO_VERT_SETBACKPACKCOLOR}",
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

Blockly.Blocks['cozmo_animation_select_menu'] = {
  /**
   * Animation drop-down menu.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "ANIMATION_SELECT",
            "options": [
              ['%{BKY_ANIM_MYSTERY}', '0'],
              ['%{BKY_ANIM_HAPPY}', '1'],
              ['%{BKY_ANIM_WINNER}', '2'],
              ['%{BKY_ANIM_SAD}', '3'],
              ['%{BKY_ANIM_SURPRISED}', '4'],
              ['%{BKY_ANIM_DOG}', '5'],
              ['%{BKY_ANIM_CAT}', '6'],
              ['%{BKY_ANIM_SNEEZE}', '7'],
              ['%{BKY_ANIM_EXCITED}', '8'],
              ['%{BKY_ANIM_THINK_HARD}', '9'],
              ['%{BKY_ANIM_BORED}', '10'],
              ['%{BKY_ANIM_FRUSTRATED}', '11'],
              ['%{BKY_ANIM_CHATTY}', '12'],
              ['%{BKY_ANIM_DISAPPOINTED}', '13'],
              ['%{BKY_ANIM_SNORE}', '14'],
              ['%{BKY_ANIM_REACT_HAPPY}', '15'],
              ['%{BKY_ANIM_CELEBRATE}', '16'],
              ['%{BKY_ANIM_TAKA_TAKA}', '17'],
              ['%{BKY_ANIM_AMAZED}', '18'],
              ['%{BKY_ANIM_CURIOUS}', '19'],
              ['%{BKY_ANIM_AGREE}', '20'],
              ['%{BKY_ANIM_DISAGREE}', '21'],
              ['%{BKY_ANIM_UNSURE}', '22'],
              ['%{BKY_ANIM_CONDUCTING}', '23'],
              ['%{BKY_ANIM_DANCING_MAMBO}', '24'],
              ['%{BKY_ANIM_FIRE_TRUCK}', '25'],
              ['%{BKY_ANIM_PARTY_TIME}', '26'],
              ['%{BKY_ANIM_DIZZY}', '27'],
              ['%{BKY_ANIM_DIZZY_END}', '28'],
              ['%{BKY_ANIM_ONE_TWO_THREE_GO}', '29'],
              ['%{BKY_ANIM_WIN_GAME}', '30'],
              ['%{BKY_ANIM_LOSE_GAME}', '31'],
              ['%{BKY_ANIM_TAP_CUBE}', '32'],
              ['%{BKY_ANIM_GET_IN_POSITION}', '33'],
              ['%{BKY_ANIM_IDLE}', '34']
            ]
          }
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.actions.primary,
        "colourSecondary": Blockly.Colours.actions.secondary,
        "colourTertiary": Blockly.Colours.actions.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
      });
  }
};

Blockly.Blocks['cozmo_vert_enable_animation_track'] = {
  /**
    * Block to enable an animation track for subsequent animations
    * @this Blockly.Block
  */
init: function() {
  this.jsonInit({
    "id": "cozmo_vert_enable_animation_track",
    "message0": "%{BKY_VERT_ENABLE_ANIMATION_TRACK}",
    "args0": [
        {
          "type": "field_dropdown",
          "name": "ANIMATION_TRACK",
          "options": [
              ['%{BKY_WHEELS}', 'wheels'],
              ['%{BKY_HEAD}', 'head'],
              ['%{BKY_LIFT}', 'lift']
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

Blockly.Blocks['cozmo_vert_disable_animation_track'] = {
  /**
    * Block to disable an animation track for subsequent animations
    * @this Blockly.Block
  */
init: function() {
  this.jsonInit({
    "id": "cozmo_vert_disable_animation_track",
    "message0": "%{BKY_VERT_DISABLE_ANIMATION_TRACK}", 
    "args0": [
        {
          "type": "field_dropdown",
          "name": "ANIMATION_TRACK",
          "options": [
              ['%{BKY_WHEELS}', 'wheels'],
              ['%{BKY_HEAD}', 'head'],
              ['%{BKY_LIFT}', 'lift']
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

Blockly.Blocks['cozmo_play_animation_from_dropdown'] = {
  /**
    * Block to make Cozmo play an animation
    * @this Blockly.Block
  */
init: function() {
  this.jsonInit({
    "id": "cozmo_play_animation_from_dropdown",
    "message0": "%{BKY_PLAY_ANIMATION}", 
    "args0": [
        {
          "type": "input_value",
          "name": "ANIMATION",
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

// FOR DEVELOPER PROTOTYPING ONLY
// This block isn't intended to ship
Blockly.Blocks['cozmo_play_animation_by_name'] = {
  /**
    * Dev-only Block to make Cozmo play a specific animation by name
    * @this Blockly.Block
  */
init: function() {
  this.jsonInit({
    "id": "cozmo_play_animation_by_name",
    "message0": "advanced: play %1 SDK animation",
    "args0": [
        {
          "type": "input_value",
          "name": "ANIM_NAME",
          "check": "String"
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

// FOR DEVELOPER PROTOTYPING ONLY
// This block isn't intended to ship
Blockly.Blocks['cozmo_play_animation_by_triggername'] = {
  /**
    * Dev-only Block to make Cozmo play a specific animation by name
    * @this Blockly.Block
  */
init: function() {
  this.jsonInit({
    "id": "cozmo_play_animation_by_triggername",
    "message0": "advanced: play %1 SDK animation group",
    "args0": [
        {
          "type": "input_value",
          "name": "TRIGGER_NAME",
          "check": "String"
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

Blockly.Blocks['cozmo_sound_sounds_menu'] = {
  /**
   * Sound effects drop-down menu.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "SOUND_MENU",
            "options": [
              ['%{BKY_EIGHTIES_MUSIC}','1'],
              ['%{BKY_MAMBO_MUSIC}','2'],
              ['%{BKY_BACKGROUND_MUSIC}','3'],
              ['%{BKY_SELECT}', '4'],
              ['%{BKY_WIN}', '5'],
              ['%{BKY_LOSE}', '6'],
              ['%{BKY_GAME_START}','7'],
              ['%{BKY_CLOCK_TICK}','8'],
              ['%{BKY_BLING}','9'],
              ['%{BKY_SUCCESS}','10'],
              ['%{BKY_FAIL}','11'],
              ['%{BKY_TIMER_WARNING}','12'],
              ['%{BKY_TIMER_END}','13'],
              ['%{BKY_SPARKLE}','14'],
              ['%{BKY_SWOOSH}','15'],
              ['%{BKY_PING}','16'],
              ['%{BKY_HOT_POTATO_END}','17'],
              ['%{BKY_HOT_POTATO_MUSIC_SLOW}','18'],
              ['%{BKY_HOT_POTATO_MUSIC_MEDIUM}','19'],
              ['%{BKY_HOT_POTATO_MUSIC_FAST}','20'],
              ['%{BKY_HOT_POTATO_MUSIC_SUPERFAST}','21'],
              ['%{BKY_MAGNET_PULL}','22'],
              ['%{BKY_MAGNET_REPEL}','23'],
              ['%{BKY_INSTRUMENT_1_MODE_1}','24'],
              ['%{BKY_INSTRUMENT_1_MODE_2}','25'],
              ['%{BKY_INSTRUMENT_1_MODE_3}','26'],
              ['%{BKY_INSTRUMENT_2_MODE_1}','27'],
              ['%{BKY_INSTRUMENT_2_MODE_2}','28'],
              ['%{BKY_INSTRUMENT_2_MODE_3}','29'],
              ['%{BKY_INSTRUMENT_3_MODE_1}','30'],
              ['%{BKY_INSTRUMENT_3_MODE_2}','31'],
              ['%{BKY_INSTRUMENT_3_MODE_3}','32']
            ]
          }
        ],
        "inputsInline": true,
        "output": "String",
        "colour": Blockly.Colours.actions.primary,
        "colourSecondary": Blockly.Colours.actions.secondary,
        "colourTertiary": Blockly.Colours.actions.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
      });
  }
};

Blockly.Blocks['cozmo_sound_play'] = {
  /**
   * Block to play sound.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_PLAY_SOUND}", 
      "args0": [
        {
          "type": "input_value",
          "name": "SOUND_MENU"
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

Blockly.Blocks['cozmo_sound_play_and_wait'] = {
  /**
   * Block to play sound and wait for it to finish playing.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_PLAY_SOUND_AND_WAIT}", 
      "args0": [
        {
          "type": "input_value",
          "name": "SOUND_MENU"
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

Blockly.Blocks['cozmo_sound_stop'] = {
  /**
   * Block to play sound.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_STOP_SOUND}", 
      "args0": [
        {
          "type": "input_value",
          "name": "SOUND_MENU"
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

Blockly.Blocks['cozmo_cube_motion_select_menu'] = {
  /**
   * Cube ID drop-down menu.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
            ]
          }
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.motion.primary,
        "colourSecondary": Blockly.Colours.motion.secondary,
        "colourTertiary": Blockly.Colours.motion.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
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
      "message0": "%{BKY_DOCK_WITH_CUBE}", 
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
      "message0": "%{BKY_DOCK_WITH_CUBE_X}",
      "args0": [
          {
            "type": "input_value",
            "name": "CUBE_SELECT",
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
    "message0": "%{BKY_SAY_X}",
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
      "message0": "%{BKY_WHEN_SEE_FACE}",
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
      "message0": "%{BKY_WHEN_SEE_HAPPY_FACE}",
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
      "message0": "%{BKY_WHEN_SEE_SAD_FACE}",
      "inputsInline": true,
      "nextStatement": null,
      "category": Blockly.Categories.event,
      "colour": Blockly.Colours.event.primary,
      "colourSecondary": Blockly.Colours.event.secondary,
      "colourTertiary": Blockly.Colours.event.tertiary
    });
  }
};

Blockly.Blocks['cozmo_cube_event_select_menu'] = {
  /**
   * Cube ID drop-down menu.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['%{BKY_ANY}', '4'],
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
            ]
          }
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.event.primary,
        "colourSecondary": Blockly.Colours.event.secondary,
        "colourTertiary": Blockly.Colours.event.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
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
      "message0": "%{BKY_WHEN_CUBE_SEEN}",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "CUBE_SELECT",
          "options": [
            ['%{BKY_ANY}', '4'],
            ['1', '1'],
            ['2', '2'],
            ['3', '3']
          ]
        }
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

Blockly.Blocks['cozmo_event_on_cube_tap'] = {
  /**
   * Block to detect that a specific cube is tapped.
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
      "id": "cozmo_event_on_cube_tap",
      "message0": "%{BKY_WHEN_CUBE_TAPPED}",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "CUBE_SELECT",
          "options": [
            ['%{BKY_ANY}', '4'],
            ['1', '1'],
            ['2', '2'],
            ['3', '3']
          ]
        }
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

Blockly.Blocks['cozmo_event_on_cube_moved'] = {
  /**
   * Block to wait until a cube moves.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_event_on_cube_moved",
      "message0": "%{BKY_WHEN_CUBE_MOVED}",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "CUBE_SELECT",
          "options": [
            ['%{BKY_ANY}', '4'],
            ['1', '1'],
            ['2', '2'],
            ['3', '3']
          ]
        }
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
        "message0": "%{BKY_COZMO_POSITION}",
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
      "message0": "%{BKY_COZMO_PITCH}",
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
      "message0": "%{BKY_COZMO_ROLL}",
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
      "message0": "%{BKY_COZMO_YAW}",
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
      "message0": "%{BKY_LIFT_HEIGHT}",
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
      "message0": "%{BKY_HEAD_ANGLE}",
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
      "message0": "%{BKY_FACE_VISIBLE}",
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
      "message0": "%{BKY_FACE_NAME}",
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
      "message0": "%{BKY_FACE_CAMERA}",
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
      "message0": "%{BKY_FACE_POSITION}",
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

Blockly.Blocks['cozmo_vert_face_get_expression'] = {
  /**
   * Block to read the expression of the most recently seen face
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_FACE_EXPRESSION}",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "String",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

// Cubes (Sensors / Inputs)

Blockly.Blocks['cozmo_cube_sensor_select_menu'] = {
  /**
   * Cube ID drop-down menu.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
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

Blockly.Blocks['cozmo_vert_cube_get_last_tapped'] = {
  /**
   * Block to read id (1,2,3) of the most recently tapped cube
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_LAST_TAPPED_CUBE}",
      "category": Blockly.Categories.sensing,
      "colour": Blockly.Colours.sensing.primary,
      "colourSecondary": Blockly.Colours.sensing.secondary,
      "colourTertiary": Blockly.Colours.sensing.tertiary,
      "output": "Boolean",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND
    });
  }
};

Blockly.Blocks['cozmo_vert_cube_get_was_tapped'] = {
  /**
   * Block to read if a given cube was recently tapped
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_CUBE_WAS_TAPPED}",
      "args0": [
        {
          "type": "input_value",
          "name": "CUBE_SELECT"
        }
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

Blockly.Blocks['cozmo_vert_cube_get_is_visible'] = {
  /**
   * Block to read if a given cube is visible
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_CUBE_IS_VISIBLE}",
      "args0": [
        {
          "type": "input_value",
          "name": "CUBE_SELECT"
        }
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
        "message0": "%{BKY_CUBE_CAMERA}",
        "args0": [
          {
            "type": "input_value",
            "name": "CUBE_SELECT"
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
        "message0": "%{BKY_CUBE_POSITION}",
        "args0": [
          {
            "type": "input_value",
            "name": "CUBE_SELECT"
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
        "message0": "%{BKY_CUBE_PITCH}",
        "args0": [
          {
            "type": "input_value",
            "name": "CUBE_SELECT"
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

Blockly.Blocks['cozmo_vert_cube_get_roll'] = {
  /**
   * Block to read the roll of one of Cozmo's cube's position
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%{BKY_CUBE_ROLL}",
        "args0": [
          {
            "type": "input_value",
            "name": "CUBE_SELECT"
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

Blockly.Blocks['cozmo_vert_cube_get_yaw'] = {
  /**
   * Block to read the yaw of one of Cozmo's cube's position
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%{BKY_CUBE_YAW}",
        "args0": [
          {
            "type": "input_value",
            "name": "CUBE_SELECT",
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

Blockly.Blocks['cozmo_vert_device_get_pitch'] = {
  /**
   * Block to read the pitch of the user's device
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_DEVICE_PITCH}",
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
      "message0": "%{BKY_DEVICE_ROLL}",
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
   * Block to read the yaw of the user's device
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_DEVICE_YAW}",
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
        "message0": "%{BKY_COZMO_VERT_DRIVE}",
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

Blockly.Blocks['cozmo_vert_wheels_speed'] = {
  /**
   * Block to drive Cozmo's wheels at different speeds
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_wheels_speed",
        "message0": "%{BKY_DRIVE_WHEELS}",
        "args0": [
          {
            "type": "input_value",
            "name": "LEFT_SPEED"
          },
          {
            "type": "input_value",
            "name": "RIGHT_SPEED"
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

Blockly.Blocks['cozmo_vert_enable_wait_for_actions'] = {
  /**
    * Block to enable automatic waiting for any action to complete before advancing to next block (default behavior)
    * @this Blockly.Block
  */
init: function() {
  this.jsonInit({
      "id": "cozmo_vert_enable_wait_for_actions",
      "message0": "%{BKY_ENABLE_WAIT_FOR_ACTIONS}",
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

Blockly.Blocks['cozmo_vert_disable_wait_for_actions'] = {
  /**
   * Block to disable automatic waiting for any action to complete before advancing to next block
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_disable_wait_for_actions",
        "message0": "%{BKY_DISABLE_WAIT_FOR_ACTIONS}",
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

Blockly.Blocks['cozmo_vert_wait_for_actions'] = {
  /**
   * Block to wait for all current actions of a given type to complete
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_wait_for_actions",
        "message0": "%{BKY_WAIT_FOR_ACTIONS}",
        "args0": [
          {
          "type": "field_dropdown",
          "name": "ACTION_SELECT",
          "options": [
            ['%{BKY_DRIVE}', 'drive'],
            ['%{BKY_HEAD}', 'head'],
            ['%{BKY_LIFT}', 'lift'],
            ['%{BKY_SAY}', 'say'],
            ['%{BKY_ANIMATION}', 'anim'],
            ['%{BKY_ALL}', 'all']
          ]
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

Blockly.Blocks['cozmo_vert_cancel_actions'] = {
  /**
   * Block to cancel all currently running actions of the given type
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cancel_actions",
        "message0": "%{BKY_STOP_ACTIONS}",
        "args0": [
          {
          "type": "field_dropdown",
          "name": "ACTION_SELECT",
          "options": [
            ['%{BKY_DRIVE}', 'drive'],
            ['%{BKY_HEAD}', 'head'],
            ['%{BKY_LIFT}', 'lift'],
            ['%{BKY_SAY}', 'say'],
            ['%{BKY_ANIMATION}', 'anim'],
            ['%{BKY_ALL}', 'all']
          ]
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

Blockly.Blocks['cozmo_vert_stop_motor'] = {
  /**
   * Block to drive Cozmo's wheels at different speeds
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_stop_motor",
        "message0": "%{BKY_STOP_MOTORS}",
        "args0": [
          {
          "type": "field_dropdown",
          "name": "MOTOR_SELECT",
          "options": [
            ['%{BKY_WHEELS}', 'wheels'],
            ['%{BKY_HEAD}', 'head'],
            ['%{BKY_LIFT}', 'lift'],
            ['%{BKY_ALL}', 'all']
          ]
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
        "message0": "%{BKY_PATH_FORWARD}",
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
        "message0": "%{BKY_PATH_TO_X_Y_ANGLE}",
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
        "message0": "%{BKY_TURN}",
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
        "message0": "%{BKY_MOVE_LIFT_TO_X}",
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

Blockly.Blocks['cozmo_vert_move_lift'] = {
  /**
   * Block to start moving the lift at the given speed
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_move_lift",
        "message0": "%{BKY_MOVE_LIFT}",
        "args0": [
          {
            "type": "input_value",
            "name": "LIFT_SPEED"
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
        "message0": "%{BKY_MOVE_HEAD_TO_X}",
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

Blockly.Blocks['cozmo_cube_looks_select_menu'] = {
  /**
   * Cube ID drop-down menu.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "CUBE_SELECT",
            "options": [
              ['1', '1'],
              ['2', '2'],
              ['3', '3']
            ]
          }
        ],
        "inputsInline": true,
        "output": "Number",
        "colour": Blockly.Colours.looks.primary,
        "colourSecondary": Blockly.Colours.looks.secondary,
        "colourTertiary": Blockly.Colours.looks.tertiary,
        "outputShape": Blockly.OUTPUT_SHAPE_ROUND
      });
  }
};

Blockly.Blocks['cozmo_vert_set_cube_light_corner'] = {
  /**
   * Block to set a cube's corner lights individually
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_vert_set_cube_light_corner",
      "message0": "%{BKY_SET_X_ON_CUBE}",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "LIGHT_SELECT",
          "options": [
            ['%{BKY_LIGHT_1}', '0'],
            ['%{BKY_LIGHT_2}', '1'],
            ['%{BKY_LIGHT_3}', '2'],
            ['%{BKY_LIGHT_4}', '3'],
            ['%{BKY_ALL_LIGHTS}', '4']
          ]
        },
        {
          "type": "input_value",
          "name": "CUBE_SELECT",
        },
        {
          "type": "input_value",
          "name": "COLOR"
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

Blockly.Blocks['cozmo_vert_cube_anim'] = {
  /**
   * Block to play a light animation on a cube
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "id": "cozmo_vert_cube_anim",
      "message0": "%{BKY_CUBE_ANIM}",
      "args0": [
        {
          "type": "field_dropdown",
          "name": "ANIM_SELECT",
          "options": [
            ['%{BKY_SPIN}', 'spin'],
            ['%{BKY_BLINK}', 'blink']
          ]
        },
        {
          "type": "input_value",
          "name": "CUBE_SELECT",
        },
        {
          "type": "input_value",
          "name": "COLOR"
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

// Draw on Cozmo's face

Blockly.Blocks['cozmo_vert_cozmoface_clear'] = {
  /**
   * Block to clear the image that will be displayed on Cozmo's face
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_clear",
        "message0": "%{BKY_DRAW_CLEAR_SCREEN}",
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};

Blockly.Blocks['cozmo_vert_cozmoface_set_draw_color'] = {
  /**
   * Block to set the color for any subsequent draw operations.
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_set_draw_color",
        "message0": "%{BKY_DRAW_COLOR}",
        "args0": [
          {
             "type": "field_dropdown",
             "name": "DRAW_COLOR",
             "options": [
                ['%{BKY_LIGHT}', 'true'],
                ['%{BKY_ERASE}', 'false']
              ]
           }
        ],
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};

Blockly.Blocks['cozmo_vert_cozmoface_set_text_scale'] = {
  /**
   * Block to set the scale for any subsequent draw text calls
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_set_text_scale",
        "message0": "%{BKY_DRAW_TEXT_SCALE}",
        "args0": [
          {
            "type": "input_value",
            "name": "SCALE"
          }
        ],
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};

Blockly.Blocks['cozmo_vert_cozmoface_set_text_alignment'] = {
  /**
   * Block to set the alignment for any subsequent draw text calls
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_set_text_alignment",
        "message0": "%{BKY_DRAW_TEXT_ALIGNMENT}",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "TEXT_ALIGNMENT_Y",
            "options": [
              ['%{BKY_TOP}', '0'],
              ['%{BKY_CENTER}', '1'],
              ['%{BKY_BOTTOM}', '2']
             ]
          },
          {
             "type": "field_dropdown",
             "name": "TEXT_ALIGNMENT_X",
             "options": [
                ['%{BKY_LEFT}', '0'],
                ['%{BKY_CENTER}', '1'],
                ['%{BKY_RIGHT}', '2']
              ]
          }
        ],
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};

Blockly.Blocks['cozmo_vert_cozmoface_draw_line'] = {
  /**
   * Block to draw a line on the image that will be displayed on Cozmo's face
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_draw_line",
        "message0": "%{BKY_DRAW_LINE}",
        "args0": [
          {
            "type": "input_value",
            "name": "X1"
          },
          {
            "type": "input_value",
            "name": "Y1"
          },
          {
            "type": "input_value",
            "name": "X2"
          },
          {
            "type": "input_value",
            "name": "Y2"
          }
        ],
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};

Blockly.Blocks['cozmo_vert_cozmoface_fill_rect'] = {
  /**
   * Block to draw a filled rectangle on the image that will be displayed on Cozmo's face
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_fill_rect",
        "message0": "%{BKY_DRAW_FILL_RECT}",
        "args0": [
          {
            "type": "input_value",
            "name": "X1"
          },
          {
            "type": "input_value",
            "name": "Y1"
          },
          {
            "type": "input_value",
            "name": "X2"
          },
          {
            "type": "input_value",
            "name": "Y2"
          }
        ],
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};

Blockly.Blocks['cozmo_vert_cozmoface_draw_rect'] = {
  /**
   * Block to draw the outline of a rectangle on the image that will be displayed on Cozmo's face
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_draw_rect",
        "message0": "%{BKY_DRAW_RECT}",
        "args0": [
          {
            "type": "input_value",
            "name": "X1"
          },
          {
            "type": "input_value",
            "name": "Y1"
          },
          {
            "type": "input_value",
            "name": "X2"
          },
          {
            "type": "input_value",
            "name": "Y2"
          }
        ],
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};

Blockly.Blocks['cozmo_vert_cozmoface_fill_circle'] = {
  /**
   * Block to draw a filled circle on the image that will be displayed on Cozmo's face
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_fill_circle",
        "message0": "%{BKY_DRAW_FILL_CIRCLE}",
        "args0": [
          {
            "type": "input_value",
            "name": "X1"
          },
          {
            "type": "input_value",
            "name": "Y1"
          },
          {
            "type": "input_value",
            "name": "RADIUS"
          }
        ],
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};

Blockly.Blocks['cozmo_vert_cozmoface_draw_circle'] = {
  /**
   * Block to draw the outline of a circle on the image that will be displayed on Cozmo's face
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_draw_circle",
        "message0": "%{BKY_DRAW_CIRCLE}",
        "args0": [
          {
            "type": "input_value",
            "name": "X1"
          },
          {
            "type": "input_value",
            "name": "Y1"
          },
          {
            "type": "input_value",
            "name": "RADIUS"
          }
        ],
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};

Blockly.Blocks['cozmo_vert_cozmoface_draw_text'] = {
  /**
   * Block to draw a string of text on the image that will be displayed on Cozmo's face
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_draw_text",
        "message0": "%{BKY_DRAW_TEXT}",
        "args0": [
          {
            "type": "input_value",
            "name": "TEXT"
          },
          {
            "type": "input_value",
            "name": "X1"
          },
          {
            "type": "input_value",
            "name": "Y1"
          }
        ],
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};


Blockly.Blocks['cozmo_vert_cozmoface_display'] = {
  /**
   * Block to display the current image (drawn with the blocks above) on Cozmo's face
   * @this Blockly.Block
  */
  init: function() {
    this.jsonInit({
        "id": "cozmo_vert_cozmoface_display",
        "message0": "%{BKY_DRAW_DISPLAY_ON_FACE}",
        "inputsInline": true,
        "previousStatement": null,
        "nextStatement": null,
        "category": Blockly.Categories.pen,
        "colour": Blockly.Colours.pen.primary,
        "colourSecondary": Blockly.Colours.pen.secondary,
        "colourTertiary": Blockly.Colours.pen.tertiary
    });
  }
};
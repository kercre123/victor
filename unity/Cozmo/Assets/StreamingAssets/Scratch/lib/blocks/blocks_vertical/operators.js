/**
 * @license
 * Visual Blocks Editor
 *
 * Copyright 2012 Google Inc.
 * https://developers.google.com/blockly/
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

goog.provide('Blockly.Blocks.operators');

goog.require('Blockly.Blocks');
goog.require('Blockly.Colours');
goog.require('Blockly.constants');
goog.require('Blockly.ScratchBlocks.VerticalExtensions');


Blockly.Blocks['operator_add'] = {
  /**
   * Block for adding two numbers.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1 + %2",
        "args0": [
          {
            "type": "input_value",
            "name": "NUM1"
          },
          {
            "type": "input_value",
            "name": "NUM2"
          }
        ],
        "category": Blockly.Categories.operators,
        "extensions": ["colours_operators", "output_number"]
      });
  }
};

Blockly.Blocks['operator_subtract'] = {
  /**
   * Block for subtracting two numbers.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1 - %2",
        "args0": [
          {
            "type": "input_value",
            "name": "NUM1"
          },
          {
            "type": "input_value",
            "name": "NUM2"
          }
        ],
        "category": Blockly.Categories.operators,
        "extensions": ["colours_operators", "output_number"]
      });
  }
};

Blockly.Blocks['operator_multiply'] = {
  /**
   * Block for multiplying two numbers.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1 * %2",
        "args0": [
          {
            "type": "input_value",
            "name": "NUM1"
          },
          {
            "type": "input_value",
            "name": "NUM2"
          }
        ],
        "category": Blockly.Categories.operators,
        "extensions": ["colours_operators", "output_number"]
      });
  }
};

Blockly.Blocks['operator_divide'] = {
  /**
   * Block for dividing two numbers.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1 / %2",
        "args0": [
          {
            "type": "input_value",
            "name": "NUM1"
          },
          {
            "type": "input_value",
            "name": "NUM2"
          }
        ],
        "category": Blockly.Categories.operators,
        "extensions": ["colours_operators", "output_number"]
      });
  }
};

Blockly.Blocks['operator_random'] = {
  /**
   * Block for picking a random number.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%{BKY_OPERATORS_PICK_RANDOM_SCRATCH_2}", // *** ANKI CHANGE ***
        "args0": [
          {
            "type": "input_value",
            "name": "FROM"
          },
          {
            "type": "input_value",
            "name": "TO"
          }
        ],
        "category": Blockly.Categories.operators,
        "extensions": ["colours_operators", "output_number"]
      });
  }
};

Blockly.Blocks['operator_lt'] = {
  /**
   * Block for less than comparator.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%1 < %2",
      "args0": [
        {
          "type": "input_value",
          "name": "OPERAND1"
        },
        {
          "type": "input_value",
          "name": "OPERAND2"
        }
      ],
      "category": Blockly.Categories.operators,
      "extensions": ["colours_operators", "output_boolean"]
    });
  }
};

Blockly.Blocks['operator_equals'] = {
  /**
   * Block for equals comparator.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%1 = %2",
      "args0": [
        {
          "type": "input_value",
          "name": "OPERAND1"
        },
        {
          "type": "input_value",
          "name": "OPERAND2"
        }
      ],
      "category": Blockly.Categories.operators,
      "extensions": ["colours_operators", "output_boolean"]
    });
  }
};

Blockly.Blocks['operator_gt'] = {
  /**
   * Block for greater than comparator.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%1 > %2",
      "args0": [
        {
          "type": "input_value",
          "name": "OPERAND1"
        },
        {
          "type": "input_value",
          "name": "OPERAND2"
        }
      ],
      "category": Blockly.Categories.operators,
      "extensions": ["colours_operators", "output_boolean"]
    });
  }
};

Blockly.Blocks['operator_and'] = {
  /**
   * Block for "and" boolean comparator.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_OPERATORS_AND_SCRATCH_2}", // *** ANKI CHANGE ***
      "args0": [
        {
          "type": "input_value",
          "name": "OPERAND1",
          "check": "Boolean"
        },
        {
          "type": "input_value",
          "name": "OPERAND2",
          "check": "Boolean"
        }
      ],
      "category": Blockly.Categories.operators,
      "extensions": ["colours_operators", "output_boolean"]
    });
  }
};

Blockly.Blocks['operator_or'] = {
  /**
   * Block for "or" boolean comparator.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_OPERATORS_OR_SCRATCH_2}", // *** ANKI CHANGE ***
      "args0": [
        {
          "type": "input_value",
          "name": "OPERAND1",
          "check": "Boolean"
        },
        {
          "type": "input_value",
          "name": "OPERAND2",
          "check": "Boolean"
        }
      ],
      "category": Blockly.Categories.operators,
      "extensions": ["colours_operators", "output_boolean"]
    });
  }
};

Blockly.Blocks['operator_not'] = {
  /**
   * Block for "not" unary boolean operator.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_OPERATORS_NOT_SCRATCH_2}", // *** ANKI CHANGE ***
      "args0": [
        {
          "type": "input_value",
          "name": "OPERAND",
          "check": "Boolean"
        }
      ],
      "category": Blockly.Categories.operators,
      "extensions": ["colours_operators", "output_boolean"]
    });
  }
};

Blockly.Blocks['operator_join'] = {
  /**
   * Block for string join operator.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_OPERATORS_JOIN_SCRATCH_2}", // *** ANKI CHANGE ***
      "args0": [
        {
          "type": "input_value",
          "name": "STRING1"
        },
        {
          "type": "input_value",
          "name": "STRING2"
        }
      ],
      "category": Blockly.Categories.operators,
      "extensions": ["colours_operators", "output_string"]
    });
  }
};

Blockly.Blocks['operator_letter_of'] = {
  /**
   * Block for "letter _ of _" operator.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_OPERATORS_LETTER_X_OF_SCRATCH_2}", // *** ANKI CHANGE ***
      "args0": [
        {
          "type": "input_value",
          "name": "LETTER"
        },
        {
          "type": "input_value",
          "name": "STRING"
        }
      ],
      "category": Blockly.Categories.operators,
      "extensions": ["colours_operators", "output_string"]
    });
  }
};

Blockly.Blocks['operator_length'] = {
  /**
   * Block for string length operator.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%{BKY_OPERATORS_LENGTH_OF_SCRATCH_2}", // *** ANKI CHANGE ***
      "args0": [
        {
          "type": "input_value",
          "name": "STRING"
        }
      ],
      "category": Blockly.Categories.operators,
      "extensions": ["colours_operators", "output_string"]
    });
  }
};

Blockly.Blocks['operator_contains'] = {
  /**
   * Block for _ contains _ operator
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%1 contains %2?",
        "args0": [
          {
            "type": "input_value",
            "name": "STRING1"
          },
          {
            "type": "input_value",
            "name": "STRING2"
          }
        ],
        "category": Blockly.Categories.operators,
        "extensions": ["colours_operators", "output_boolean"]
      });
  }
};

Blockly.Blocks['operator_mod'] = {
  /**
   * Block for mod two numbers.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%{BKY_OPERATORS_MOD_SCRATCH_2}", // *** ANKI CHANGE ***
        "args0": [
          {
            "type": "input_value",
            "name": "NUM1"
          },
          {
            "type": "input_value",
            "name": "NUM2"
          }
        ],
        "category": Blockly.Categories.operators,
        "extensions": ["colours_operators", "output_number"]
      });
  }
};

Blockly.Blocks['operator_round'] = {
  /**
   * Block for rounding a numbers.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%{BKY_OPERATORS_ROUND_SCRATCH_2}", // *** ANKI CHANGE ***
        "args0": [
          {
            "type": "input_value",
            "name": "NUM"
          }
        ],
        "category": Blockly.Categories.operators,
        "extensions": ["colours_operators", "output_number"]
      });
  }
};

Blockly.Blocks['operator_mathop'] = {
  /**
   * Block for "advanced" math ops on a number.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit(
      {
        "message0": "%{BKY_OPERATORS_MATH_OP_SCRATCH_2}",
        "args0": [
          {
            "type": "field_dropdown",
            "name": "OPERATOR",
            "options": [
              ['%{BKY_OPERATORS_ABS_SCRATCH_2}', 'abs'],
              ['%{BKY_OPERATORS_FLOOR_SCRATCH_2}', 'floor'],
              ['%{BKY_OPERATORS_CEILING_SCRATCH_2}', 'ceiling'],
              ['%{BKY_OPERATORS_SQRT_SCRATCH_2}', 'sqrt'],
              ['%{BKY_OPERATORS_SIN_SCRATCH_2}', 'sin'],
              ['%{BKY_OPERATORS_COS_SCRATCH_2}', 'cos'],
              ['%{BKY_OPERATORS_TAN_SCRATCH_2}', 'tan'],
              ['%{BKY_OPERATORS_ASIN_SCRATCH_2}', 'asin'],
              ['%{BKY_OPERATORS_ACOS_SCRATCH_2}', 'acos'],
              ['%{BKY_OPERATORS_ATAN_SCRATCH_2}', 'atan'],
              ['%{BKY_OPERATORS_LN_SCRATCH_2}', 'ln'],
              ['%{BKY_OPERATORS_LOG_SCRATCH_2}', 'log'],
              ['%{BKY_OPERATORS_EULER_MULT_SCRATCH_2}', 'e ^'],
              ['%{BKY_OPERATORS_TEN_MULT_SCRATCH_2}', '10 ^']
            ]
          },
          {
            "type": "input_value",
            "name": "NUM"
          }
        ],
        "category": Blockly.Categories.operators,
        "extensions": ["colours_operators", "output_number"]
      });
  }
};

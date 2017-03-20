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

/**
 * @fileoverview Text blocks for Blockly.
 * @author fraser@google.com (Neil Fraser)
 */
'use strict';

goog.provide('Blockly.Blocks.texts');

goog.require('Blockly.Blocks');

goog.require('Blockly.Colours');

goog.require('Blockly.constants');

var MAX_TEXT_LENGTH = 32;

Blockly.Blocks['text'] = {
  /**
   * Block for text value.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%1",
      "args0": [
        {
          "type": "field_input",
          "name": "TEXT"
        }
      ],
      "output": "String",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND,
      "colour": Blockly.Colours.textField
    });
  }
};

// Same as text block above, with a validator added
Blockly.Blocks['text_with_validator'] = {
  /**
   * Block for text value.
   * @this Blockly.Block
   */
  init: function() {
    this.jsonInit({
      "message0": "%1",
      "args0": [
        {
          "type": "field_input",
          "name": "TEXT"
        }
      ],
      "output": "String",
      "outputShape": Blockly.OUTPUT_SHAPE_ROUND,
      "colour": Blockly.Colours.textField
    });

    // Add validator for cozmo_says block to limit the length of the string to
    // MAX_TEXT_LENGTH characters.
    this.getField("TEXT").setValidator(function(inputText) {
      if (inputText.length > MAX_TEXT_LENGTH) {
        inputText = inputText.slice(0, MAX_TEXT_LENGTH);
      }
      return inputText;
    });
  }
};

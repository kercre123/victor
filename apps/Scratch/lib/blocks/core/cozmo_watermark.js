/**
 *
 * *** ANKI CHANGE ***
 *
 * Adds CozmoWatermark class so that the watermark image can
 * be added to the workspace and blocks will sit on top of it.
 * The z-index setting in css is not respected. Note from block_svg.js:
 *
 *     <g> tags do not respect z-index so svg renders them in the
 *     order that they are in the dom.  By placing this block first within the
 *     block group's <g>, it will render on top of any other blocks.
 * 
 * By adding CozmoWatermark from workspace_svg.js with HTML DOM memthod
 * insertBefore(), with the SVG <g> tags, the watermark node is inserted
 * early in the DOM and the blocks remain on top of the watermark after
 * they are dropped by the user.
 *
 * @fileoverview Object representing a Cozmo Watermark.
 */
'use strict';

goog.provide('Blockly.CozmoWatermark');

goog.require('goog.dom');
goog.require('goog.math');


/**
 * Class for a Cozmo watermark.
 * @param {!Blockly.Workspace} workspace The workspace to sit in.
 * @constructor
 */
Blockly.CozmoWatermark = function(workspace) {
  this.workspace_ = workspace;
};

// Default sizing is for tablet
Blockly.CozmoWatermark.prototype.watermarkWidth_ = 430;
Blockly.CozmoWatermark.prototype.watermarkHeight_ = 69;

/**
 * The SVG group containing the watermark.
 * @type {Element}
 * @private
 */
Blockly.CozmoWatermark.prototype.svgGroup_ = null;

/**
 * Left coordinate of the watermark.
 * @type {number}
 * @private
 */
Blockly.CozmoWatermark.prototype.left_ = 0;

/**
 * Top coordinate of the watermark.
 * @type {number}
 * @private
 */
Blockly.CozmoWatermark.prototype.top_ = 0;


/**
 * Create the watermark.
 * @return {!Element} The watermark's SVG group.
 */
Blockly.CozmoWatermark.prototype.createDom = function() {
  if (window.innerWidth < window.TABLET_WIDTH) {
    // Size for phone, 60% of tablet size. Note that this method
    // can be called more than once and is called before init.
    Blockly.CozmoWatermark.prototype.watermarkWidth_ = 258;
    Blockly.CozmoWatermark.prototype.watermarkHeight_ = 42;
  }

  var watermarkFilePath = "./images/ui/";
  var watermarkFileName = "code_lab_watermark.svg";
  var cozmoLocale = window.getUrlVars()['locale'];
  if (cozmoLocale == "ja-JP") {
    watermarkFileName =  "code_lab_watermark_JP.svg"
  }

  /* Generates markup like:
  <g class="blocklyWatermark">
    <image width="430" x="0" height="69" y="0" xlink:href=./images/ui/code_lab_watermark.svg"></image>
  </g>
  */

  this.svgGroup_ = Blockly.utils.createSvgElement('g',
      {'class': 'blocklyWatermark'}, null);
  var body = Blockly.utils.createSvgElement('image',
      {'width': Blockly.CozmoWatermark.prototype.watermarkWidth_, 'x': 0,
       'height': Blockly.CozmoWatermark.prototype.watermarkHeight_, 'y': 0},
      this.svgGroup_);
  body.setAttributeNS('http://www.w3.org/1999/xlink', 'xlink:href',
     watermarkFilePath + watermarkFileName);

  return this.svgGroup_;
};

/**
 * Dispose of the watermark.
 * Unlink from all DOM elements to prevent memory leaks.
 */
Blockly.CozmoWatermark.prototype.dispose = function() {
  if (this.svgGroup_) {
    goog.dom.removeNode(this.svgGroup_);
    this.svgGroup_ = null;
  }
  this.workspace_ = null;
};

/**
 * Center the watermark.
 */
Blockly.CozmoWatermark.prototype.position = function() {
  var injectionDiv = this.workspace_.getInjectionDiv();
  if (!injectionDiv) {
    return;
  }

  var boundingRect = injectionDiv.getBoundingClientRect();
  this.top_ = (boundingRect.height - Blockly.CozmoWatermark.prototype.watermarkHeight_)/2;
  this.left_ = (boundingRect.width - Blockly.CozmoWatermark.prototype.watermarkWidth_)/2;
  this.svgGroup_.setAttribute('transform',
      'translate(' + this.left_ + ',' + this.top_ + ')');
};

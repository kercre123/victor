var Cast = require('../util/cast');
var MathUtil = require('../util/math-util');
var Timer = require('../util/timer');

var Scratch3CozmoBlocks = function (runtime) {
    /**
     * The runtime instantiating this block package.
     * @type {Runtime}
     */
    this.runtime = runtime;

    this._currentRequestId = 0;
};

/**
 * Retrieve the block primitives implemented by this package.
 * @return {Object.<string, Function>} Mapping of opcode to Function.
 */
Scratch3CozmoBlocks.prototype.getPrimitives = function () {
    return {
        cozmo_setbackpackcolor: this.setBackpackColor,
        cozmo_drive_forward: this.driveForward,
        cozmo_drive_backward: this.driveBackward,
        cozmo_animation: this.playAnimation,
        cozmo_liftheight: this.setLiftHeight,
        cozmo_wait_for_face: this.waitUntilSeeFace,
        cozmo_wait_until_see_cube: this.waitUntilSeeCube,
        cozmo_wait_for_cube_tap: this.waitForCubeTap,
        cozmo_headangle: this.setHeadAngle,
        cozmo_dock_with_cube: this.dockWithCube,
        cozmo_turn_left: this.turnLeft,
        cozmo_turn_right: this.turnRight,
        cozmo_drive_speed: this.driveSpeed,
        cozmo_says: this.speak
    };
};

// Values returned will be from 0 to 126.
//
// In the VM, the methods called by the Cozmo blocks (e.g., Scratch3CozmoBlocks.prototype.driveForward)
// each generate a ScratchRequest.requestId (by calling _getRequestId()). In addition, these methods
// each return a Promise, with the corresponding resolve function stored in the window.resolveCommands array.
Scratch3CozmoBlocks.prototype._getRequestId = function () {
    this._currentRequestId += 1;
    this._currentRequestId %= 127;
    return this._currentRequestId;
};

Scratch3CozmoBlocks.prototype._promiseForCommand = function (requestId) {
    return new Promise(function (resolve) {
        window.resolveCommands[requestId] = resolve;
    });
};

Scratch3CozmoBlocks.prototype.setBackpackColor = function(args, util) {
    // Cozmo performs this request instantly, so we have a timeout instead in js
    // so the user can see each backpack light setting.
    //
    // Pass -1 for requestId to indicate we don't need a Promise resolved.
    if (!util.stackFrame.timer) {
        var colorHexValue = this._getColor(Cast.toString(args.CHOICE));
        window.Unity.call('{"requestId": "' + -1 + '", "command": "cozmoSetBackpackColor","argUInt": "' + colorHexValue + '"}');
        
        // Yield
        util.stackFrame.timer = new Timer();
        util.stackFrame.timer.start();
        util.yield();
    } else {
        if (util.stackFrame.timer.timeElapsed() < 500) {
            util.yield();
        }
    }
};

Scratch3CozmoBlocks.prototype.driveForward = function(args, util) {
    if (args.DISTANCE < 1) {
        return;
    }

    // The distMultiplier (as set by the parameter number under the block)
    // will be used as a multiplier against the base dist_mm.
    var distMultiplier = Cast.toNumber(args.DISTANCE);
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoDriveForward","argFloat": ' + distMultiplier + ', "argString": "' + this.runtime.cozmoDriveSpeed + '"}');

    return this._promiseForCommand(requestId);
};

Scratch3CozmoBlocks.prototype.driveBackward = function(args, util) {
    if (args.DISTANCE < 1) {
        return;
    }

    // The distMultiplier (as set by the parameter number under the block)
    // will be used as a multiplier against the base dist_mm.
    var distMultiplier = Cast.toNumber(args.DISTANCE);
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoDriveBackward","argFloat": ' + distMultiplier + ', "argString": "' + this.runtime.cozmoDriveSpeed + '"}');

    return this._promiseForCommand(requestId);
};

Scratch3CozmoBlocks.prototype.playAnimation = function(args, util) {
    var animationName = this._getAnimation(Cast.toString(args.CHOICE));
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoPlayAnimation","argString": "' + animationName + '"}');

    return this._promiseForCommand(requestId);
};

Scratch3CozmoBlocks.prototype.setLiftHeight = function(args, util) {
    var liftHeight = Cast.toString(args.CHOICE);
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoForklift","argString": "' + liftHeight + '"}');

    return this._promiseForCommand(requestId);
};    

Scratch3CozmoBlocks.prototype.setHeadAngle = function(args, util) {
    var headAngle = Cast.toString(args.CHOICE);
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoHeadAngle","argString": "' + headAngle + '"}');

    return this._promiseForCommand(requestId);
};

Scratch3CozmoBlocks.prototype.dockWithCube = function(args, util) {
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoDockWithCube"}');

    return this._promiseForCommand(requestId);
};
                                       
Scratch3CozmoBlocks.prototype.turnLeft = function(args, util) {
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoTurnLeft"}');

    return this._promiseForCommand(requestId);
};

Scratch3CozmoBlocks.prototype.turnRight = function(args, util) {
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoTurnRight"}');

    return this._promiseForCommand(requestId);
};

Scratch3CozmoBlocks.prototype.speak = function(args, util) {
    var textToSay = Cast.toString(args.STRING);
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoSays","argString": "' + textToSay + '"}');

    return this._promiseForCommand(requestId);
};

/**
 * Wait until see face.
 *
 * @param argValues Parameters passed with the block.
 * @param util The util instance to use for yielding and finishing.
 * @private
 */
Scratch3CozmoBlocks.prototype.waitUntilSeeFace = function (args, util) {
    // For now pass -1 for requestId to indicate we don't need a Promise resolved.
    window.Unity.call('{"requestId": "' + -1 + '", "command": "cozmoHeadAngle","argString": "high"}');

    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoWaitUntilSeeFace"}');

    return this._promiseForCommand(requestId);
};

/**
 * Wait until see cube.
 *
 * @param argValues Parameters passed with the block.
 * @param util The util instance to use for yielding and finishing.
 * @private
 */
Scratch3CozmoBlocks.prototype.waitUntilSeeCube = function (args, util) {
    // For now pass -1 for requestId to indicate we don't need a Promise resolved.
    window.Unity.call('{"requestId": "' + -1 + '", "command": "cozmoHeadAngle","argString": "low"}');

    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoWaitUntilSeeCube"}');

    return this._promiseForCommand(requestId);
};

/**
 * Wait until cube is tapped.
 *
 * @param argValues Parameters passed with the block.
 * @param util The util instance to use for yielding and finishing.
 * @private
 */
Scratch3CozmoBlocks.prototype.waitForCubeTap = function (args, util) {
    var requestId = this._getRequestId();
    window.Unity.call('{"requestId": "' + requestId + '", "command": "cozmoWaitForCubeTap"}');

    return this._promiseForCommand(requestId);
};

Scratch3CozmoBlocks.prototype.driveSpeed = function(args, util) {
    this.runtime.cozmoDriveSpeed = Cast.toString(args.CHOICE);
};

/**
 * Convert a color name to a Cozmo color index.
 * Supports 'mystery' for a random hue.
 * @param colorName The color to retrieve.
 * @returns {number} The Cozmo color index.
 * @private
 */
Scratch3CozmoBlocks.prototype._getColor = function(colorName) {
    var colorNameToHexTable = [
        {colorName: 'yellow', colorHex: 0xffff00ff},
        {colorName: 'orange', colorHex: 0xffA500ff},
        {colorName: 'coral', colorHex: 0xff0000ff},
        {colorName: 'purple', colorHex: 0xff00ffff},
        {colorName: 'blue', colorHex: 0x0000ffff},
        {colorName: 'green', colorHex: 0x00ff00ff},
        {colorName: 'white', colorHex: 0xffffffff},
        {colorName: 'off', colorHex: 0x00000000}
    ];

    if (colorName == 'mystery') {
        // Don't allow black to be an option that can be selected
        var randomValue = Math.floor(Math.random() * (colorNameToHexTable.length-1));
        return colorNameToHexTable[randomValue].colorHex;
    }

    var colorHexToReturn;
    for (var i = 0; i < colorNameToHexTable.length; i++) {
        if (colorNameToHexTable[i].colorName == colorName)
        {
            colorHexToReturn = colorNameToHexTable[i].colorHex;
        }
    }

    return colorHexToReturn;
};

/**
 * Convert a string to a Cozmo animation trigger index.
 * Supports 'mystery' for a random animation.
 * @param animationName The animation to retrieve.
 * @returns {number} The Cozmo animation trigger index.
 * @private
 */
Scratch3CozmoBlocks.prototype._getAnimation = function(animationName) {
    var animationTable = [
        'happy',
        'victory',
        'unhappy',
        'surprise',
        'dog',
        'cat',
        'sneeze',
        'excited',
        'thinking',
        'bored',
        'frustrated',
        'chatty',
        'dejected',
        'sleep'
    ];

    if (animationName == 'mystery') {
        var randomValue = Math.floor(Math.random() * animationTable.length);
        return animationTable[randomValue];
    }

    return animationName;
};

module.exports = Scratch3CozmoBlocks;

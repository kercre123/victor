var Cast = require('../util/cast');
var MathUtil = require('../util/math-util');
var Timer = require('../util/timer');

var Scratch3CozmoBlocks = function (runtime) {
    /**
     * The runtime instantiating this block package.
     * @type {Runtime}
     */
    this.runtime = runtime;

    /**
     * Current motor speed as a percentage (100 = full speed).
     * @type {Number}
     */
    this._motorSpeed = 100;
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
        cozmo_wait_for_face: this.waitForFace,
        cozmo_wait_for_cube: this.waitForCube,
        cozmo_headangle: this.setHeadAngle,
        cozmo_turn_left: this.turnLeft,
        cozmo_turn_right: this.turnRight,
        cozmo_drive_speed: this.driveSpeed,
        cozmo_says: this.speak
    };
};

Scratch3CozmoBlocks.prototype.setBackpackColor = function(args, util) {
    // TODO Wait for RobotCompletedAction instead of using timer.
    if (!util.stackFrame.timer) {
        var colorHexValue = this._getColor(Cast.toString(args.CHOICE));
        window.Unity.call('{"command": "cozmoSetBackpackColor","argUInt": "' + colorHexValue + '"}');
        
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
    // TODO Wait for RobotCompletedAction instead of using timer.
    if (!util.stackFrame.timer) {
        // The distMultiplier will be between 1 and 9 (as set by the parameter
        // number under the block) and will be used as a multiplier against the
        // base dist_mm of 30.0f.
        var distMultiplier = Cast.toNumber(args.DISTANCE);
        window.Unity.call('{"command": "cozmoDriveForward","argFloat": ' + distMultiplier + ', "argString": "' + this.runtime.cozmoDriveSpeed + '"}');

        // Yield
        util.stackFrame.timer = new Timer();
        util.stackFrame.timer.start();
        util.yield();
    } else {
        // TODO Add distance as multiplier to the time below
        if (util.stackFrame.timer.timeElapsed() < 1000) {
            util.yield();
        }
    }
};

Scratch3CozmoBlocks.prototype.driveBackward = function(args, util) {
    // TODO Wait for RobotCompletedAction instead of using timer.
    if (!util.stackFrame.timer) {
        // The distMultiplier will be between 1 and 9 (as set by the parameter
        // number under the block) and will be used as a multiplier against the
        // base dist_mm of 30.0f.
        var distMultiplier = Cast.toNumber(args.DISTANCE);
        window.Unity.call('{"command": "cozmoDriveBackward","argFloat": ' + distMultiplier + ', "argString": "' + this.runtime.cozmoDriveSpeed + '"}');

        // Yield
        util.stackFrame.timer = new Timer();
        util.stackFrame.timer.start();
        util.yield();
    } else {
        // TODO Add distance as multiplier to the time below
        if (util.stackFrame.timer.timeElapsed() < 1000) {
            util.yield();
        }
    }
};

Scratch3CozmoBlocks.prototype.playAnimation = function(args, util) {
    // TODO Wait for RobotCompletedAction instead of using timer.
    if (!util.stackFrame.timer) {
        var animationName = this._getAnimation(Cast.toString(args.CHOICE));
        window.Unity.call('{"command": "cozmoPlayAnimation","argString": "' + animationName + '"}');

        // Yield
        util.stackFrame.timer = new Timer();
        util.stackFrame.timer.start();
        util.yield();
    } else {
        // TODO Is this long enough for all animations?
        if (util.stackFrame.timer.timeElapsed() < 3000) {
            util.yield();
        }
    }
};

Scratch3CozmoBlocks.prototype.setLiftHeight = function(args, util) {
    // TODO Wait for RobotCompletedAction instead of using timer.
    if (!util.stackFrame.timer) {
        var liftHeight = Cast.toString(args.CHOICE);
        window.Unity.call('{"command": "cozmoForklift","argString": "' + liftHeight + '"}');

        // Yield
        util.stackFrame.timer = new Timer();
        util.stackFrame.timer.start();
        util.yield();
    } else {
        if (util.stackFrame.timer.timeElapsed() < 1000) {
            util.yield();
        }
    }
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
        var randomValue = Math.floor(Math.random() * colorNameToHexTable.length);
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

/**
 * The waitForFace and waitForCube methods work the same way as the
 * control cube waitUntil. Using waitForFace as an example:
 *
 * First, waitForFace is called.
 *
 * waitForFace calls util.yield(), which sets thread.status to STATUS_YIELD.
 *
 * stepThread() is called. In stepThread(), the code sees that the thread status
 * is STATUS_YIELD and sets it to STATUS_RUNNING. stepThread returns.
 *
 * The vm calls waitForFace again. If the condition we are checking for
 * still exists (in this case the condition is “has a face been seen),
 * then again the thread is set to STATUS_YIELD. Otherwise, if a face
 * has been seen, then the function finishes, leaving the thread status
 * as STATUS_RUNNING.
 *
 * The next time stepThread is called, it calls goToNextBlock, so that the
 * block following the waitForFace block is executed.
 *
 * @param argValues Parameters passed with the block.
 * @param util The util instance to use for yielding and finishing.
 * @private
 */
Scratch3CozmoBlocks.prototype.waitForFace = function (args, util) {
    this.runtime.stackIsWaitingForFace = true;
    if (!this.runtime.cozmoSawFace) {
        util.yield();
    }
    else {
        this.runtime.cozmoSawFace = false;
        this.runtime.stackIsWaitingForFace = false;
    }
};

/**
 * See waitForFace docs above.
 *
 * @param argValues Parameters passed with the block.
 * @param util The util instance to use for yielding and finishing.
 * @private
 */
Scratch3CozmoBlocks.prototype.waitForCube = function (args, util) {
    this.runtime.stackIsWaitingForCube = true;
    if (!this.runtime.cozmoSawCube) {
        util.yield();
    }
    else {
        this.runtime.cozmoSawCube = false;
        this.runtime.stackIsWaitingForCube = false;
    }
};

Scratch3CozmoBlocks.prototype.setHeadAngle = function(args, util) {
    // TODO Wait for RobotCompletedAction instead of using timer.
    if (!util.stackFrame.timer) {
        var headAngle = Cast.toString(args.CHOICE);
        window.Unity.call('{"command": "cozmoHeadAngle","argString": "' + headAngle + '"}');

        // Yield
        util.stackFrame.timer = new Timer();
        util.stackFrame.timer.start();
        util.yield();
    } else {
        if (util.stackFrame.timer.timeElapsed() < 1000) {
            util.yield();
        }
    }
};
                                       
Scratch3CozmoBlocks.prototype.turnLeft = function(args, util) {
    // TODO Wait for RobotCompletedAction instead of using timer.
    if (!util.stackFrame.timer) {
        window.Unity.call('{"command": "cozmoTurnLeft"}');

        // Yield
        util.stackFrame.timer = new Timer();
        util.stackFrame.timer.start();
        util.yield();
    } else {
        if (util.stackFrame.timer.timeElapsed() < 1000) {
            util.yield();
        }
    }
};

Scratch3CozmoBlocks.prototype.turnRight = function(args, util) {
    // TODO Wait for RobotCompletedAction instead of using timer.
    if (!util.stackFrame.timer) {
        window.Unity.call('{"command": "cozmoTurnRight"}');

        // Yield
        util.stackFrame.timer = new Timer();
        util.stackFrame.timer.start();
        util.yield();
    } else {
        if (util.stackFrame.timer.timeElapsed() < 1000) {
            util.yield();
        }
    }
};

Scratch3CozmoBlocks.prototype.driveSpeed = function(args, util) {
    this.runtime.cozmoDriveSpeed = Cast.toString(args.CHOICE);
};

Scratch3CozmoBlocks.prototype.speak = function(args, util) {
    // TODO Wait for RobotCompletedAction instead of using timer.
    if (!util.stackFrame.timer) {
        var textToSay = Cast.toString(args.STRING);
        window.Unity.call('{"command": "cozmoSays","argString": "' + textToSay + '"}');

        // Yield
        util.stackFrame.timer = new Timer();
        util.stackFrame.timer.start();
        util.yield();
    } else {
        // TODO Add length of string as multiplier to the time below?
        if (util.stackFrame.timer.timeElapsed() < 2000) {
            util.yield();
        }
    }
};

module.exports = Scratch3CozmoBlocks;

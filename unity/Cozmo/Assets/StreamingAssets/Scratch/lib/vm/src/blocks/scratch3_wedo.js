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
        cozmo_drive: this.driveForward,
        cozmo_animation: this.playAnimation,
        cozmo_liftheight: this.setLiftHeight,
        cozmo_wait_for_face: this.waitForFace,
        cozmo_wait_for_cube: this.waitForCube,
        cozmo_headangle: this.setHeadAngle,
        cozmo_turn: this.turn,
        cozmo_says: this.speak
    };
};

Scratch3CozmoBlocks.prototype.setBackpackColor = function(args, util) {
    if (!util.stackFrame.timer) {
        function callNativeApp(msg) {
            try {
                webkit.messageHandlers.callbackHandler.postMessage(msg);
            } catch(err) {
                console.log("Native context is missing.");
            }
        }

        var colorIndex = this._getColor(Cast.toString(args.CHOICE));
        var jsonValues = {
            "command": "cozmoSetBackpackColor",
            "color": colorIndex
        };
        callNativeApp(jsonValues);
        
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
    if (!util.stackFrame.timer) {
       function callNativeApp(msg) {
           try {
               webkit.messageHandlers.callbackHandler.postMessage(msg);
           } catch(err) {
               console.log("Native context is missing.");
           }
       }

       // The dist_multiplier will be between 1 and 9 (as set by the parameter
       // number under the block) and will be used as a multiplier against the
       // base dist_mm of 30.0f.
       var jsonValues = {
           "command": "cozmoDriveForward",
           "dist_multiplier": Cast.toNumber(args.DURATION)
       };
       callNativeApp(jsonValues);

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
    if (!util.stackFrame.timer) {
        function callNativeApp(msg) {
            try {
                webkit.messageHandlers.callbackHandler.postMessage(msg);
            } catch(err) {
                console.log("Native context is missing.");
            }
        }

        var jsonValues = {
            "command": "cozmoPlayAnimation"
        };
        callNativeApp(jsonValues);

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
    if (!util.stackFrame.timer) {
       function callNativeApp(msg) {
           try {
               webkit.messageHandlers.callbackHandler.postMessage(msg);
           } catch(err) {
               console.log("Native context is missing.");
           }
       }

       var jsonValues = {
           "command": "cozmoForklift",
           "liftHeight": Cast.toString(args.CHOICE)
        };
        callNativeApp(jsonValues);

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
 * Convert a color name to a WeDo color index.
 * Supports 'mystery' for a random hue.
 * @param colorName The color to retrieve.
 * @returns {number} The WeDo color index.
 * @private
 */
Scratch3CozmoBlocks.prototype._getColor = function(colorName) {
    var colors = {
        'yellow': 7,
        'orange': 8,
        'coral': 9,
        'magenta': 1,
        'purple': 2,
        'blue': 3,
        'green': 6,
        'white': 10
    };

    if (colorName == 'mystery') {
        return Math.floor((Math.random() * 10) + 1);
    }

    return colors[colorName];
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
    if (!util.stackFrame.timer) {
        function callNativeApp(msg) {
            try {
                webkit.messageHandlers.callbackHandler.postMessage(msg);
            } catch(err) {
                console.log("Native context is missing.");
            }
        }
   
        var jsonValues = {
            "command": "cozmoHeadAngle",
            "headAngle": Cast.toString(args.CHOICE)
        };
        callNativeApp(jsonValues);

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
                                       
Scratch3CozmoBlocks.prototype.turn = function(args, util) {
    if (!util.stackFrame.timer) {
        function callNativeApp(msg) {
            try {
                webkit.messageHandlers.callbackHandler.postMessage(msg);
            } catch(err) {
                console.log("Native context is missing.");
            }
        }

        var jsonValues = {
            "command": "cozmoTurn"
        };
        callNativeApp(jsonValues);

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

Scratch3CozmoBlocks.prototype.speak = function(args, util) {
    if (!util.stackFrame.timer) {
        function callNativeApp(msg) {
            try {
                webkit.messageHandlers.callbackHandler.postMessage(msg);
            } catch(err) {
                console.log("Native context is missing.");
            }
        }
                                       
        var jsonValues = {
            "command": "cozmoSays",
            "text": Cast.toString(args.STRING)
        };
        callNativeApp(jsonValues);

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

var Cast = require('../util/cast');
const Color = require('../util/color');
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
        cozmo_drive_forward_fast: this.driveForwardFast,
        cozmo_drive_backward: this.driveBackward,
        cozmo_drive_backward_fast: this.driveBackwardFast,
        cozmo_happy_animation: this.playHappyAnimation,
        cozmo_victory_animation: this.playVictoryAnimation,
        cozmo_unhappy_animation: this.playUnhappyAnimation,
        cozmo_surprise_animation: this.playSurpriseAnimation,
        cozmo_dog_animation: this.playDogAnimation,
        cozmo_cat_animation: this.playCatAnimation,
        cozmo_sneeze_animation: this.playSneezeAnimation,
        cozmo_excited_animation: this.playExcitedAnimation,
        cozmo_thinking_animation: this.playThinkingAnimation,
        cozmo_bored_animation: this.playBoredAnimation,
        cozmo_frustrated_animation: this.playFrustratedAnimation,
        cozmo_chatty_animation: this.playChattyAnimation,
        cozmo_dejected_animation: this.playDejectedAnimation,
        cozmo_sleep_animation: this.playSleepAnimation,
        cozmo_mystery_animation: this.playMysteryAnimation,
        cozmo_liftheight: this.setLiftHeight,
        cozmo_wait_for_face: this.waitUntilSeeFace,
        cozmo_wait_for_happy_face: this.waitUntilSeeHappyFace,
        cozmo_wait_for_sad_face: this.waitUntilSeeSadFace,
        cozmo_wait_until_see_cube: this.waitUntilSeeCube,
        cozmo_wait_for_cube_tap: this.waitForCubeTap,
        cozmo_headangle: this.setHeadAngle,
        cozmo_dock_with_cube: this.dockWithCube,
        cozmo_turn_left: this.turnLeft,
        cozmo_turn_right: this.turnRight,
        cozmo_says: this.speak,
        // ==================== Vertical Grammar ====================
        cozmo_vert_setbackpackcolor: this.verticalSetBackpackColor,
        // Actions
        cozmo_play_animation_from_dropdown: this.playAnimationFromDropdown,
        // Begin temporary dev-only blocks prototyping
        cozmo_play_animation_by_name: this.playAnimationByName,
        cozmo_play_animation_by_triggername: this.playAnimationByTriggerName,
        // End temporary dev-only blocks prototyping
        cozmo_vert_enable_animation_track: this.verticalEnableAnimationTrack,
        cozmo_vert_disable_animation_track: this.verticalDisableAnimationTrack,
        cozmo_vert_enable_wait_for_actions: this.verticalEnableWaitForActions,
        cozmo_vert_disable_wait_for_actions: this.verticalDisableWaitForActions,
        cozmo_vert_wait_for_actions: this.verticalWaitForActions,
        cozmo_vert_cancel_actions: this.verticalCancelActions,
        cozmo_vert_turn: this.verticalTurn,
        cozmo_vert_drive: this.verticalDrive,
        cozmo_vert_wheels_speed: this.verticalDriveWheels,
        cozmo_vert_stop_motor: this.verticalStopMotor,
        cozmo_vert_path_offset: this.verticalPathOffset,
        cozmo_vert_path_to: this.verticalPathTo,
        cozmo_vert_set_headangle: this.verticalSetHeadAngle,
        cozmo_vert_set_liftheight: this.verticalSetLiftHeight,
        cozmo_vert_move_lift: this.verticalMoveLift,
        cozmo_vert_dock_with_cube_by_id: this.verticalDockWithCubeById,
        cozmo_vert_set_cube_light_corner: this.verticalSetCubeLightCorner,
        cozmo_vert_cube_anim: this.verticalCubeAnim,
        cozmo_sound_play: this.verticalPlaySound,
        cozmo_sound_play_and_wait: this.verticalPlaySoundAndWait,
        cozmo_sound_stop: this.verticalStopSound,
        // Draw (on Cozmo's face)
        cozmo_vert_cozmoface_clear: this.verticalCozmoFaceClear,
        cozmo_vert_cozmoface_display: this.verticalCozmoFaceDisplay,
        cozmo_vert_cozmoface_set_draw_color: this.verticalCozmoFaceSetDrawColor,
        cozmo_vert_cozmoface_set_text_scale: this.verticalCozmoFaceSetTextScale,
        cozmo_vert_cozmoface_set_text_alignment: this.verticalCozmoFaceSetTextAlignment,
        cozmo_vert_cozmoface_draw_line: this.verticalCozmoFaceDrawLine,
        cozmo_vert_cozmoface_fill_rect: this.verticalCozmoFaceFillRect,
        cozmo_vert_cozmoface_draw_rect: this.verticalCozmoFaceDrawRect,
        cozmo_vert_cozmoface_fill_circle: this.verticalCozmoFaceFillCircle,
        cozmo_vert_cozmoface_draw_circle: this.verticalCozmoFaceDrawCircle,
        cozmo_vert_cozmoface_draw_text: this.verticalCozmoFaceDrawText,
        // ================
        // Sensors / Inputs
        // Cozmo
        cozmo_vert_get_position_3d: this.verticalCozmoGetPosition,
        cozmo_vert_get_pitch: this.verticalCozmoGetPitch,
        cozmo_vert_get_roll: this.verticalCozmoGetRoll,
        cozmo_vert_get_yaw: this.verticalCozmoGetYaw,
        cozmo_vert_get_lift_height: this.verticalCozmoGetLiftHeight,
        cozmo_vert_get_head_angle: this.verticalCozmoGetHeadAngle,
        cozmo_vert_get_is_picked_up: this.verticalCozmoGetIsPickedUp,
        // Faces
        cozmo_vert_face_get_is_visible: this.verticalFaceGetIsVisible,
        cozmo_vert_face_get_name: this.verticalFaceGetName,
        cozmo_vert_face_get_position_2d: this.verticalFaceGet2d,
        cozmo_vert_face_get_position_3d: this.verticalFaceGet3d,
        cozmo_vert_face_get_expression: this.verticalFaceGetExpression,
        // Cubes
        cozmo_vert_cube_get_last_tapped: this.verticalCubeGetLastTapped,
        cozmo_vert_cube_get_was_tapped: this.verticalCubeGetWasTapped,
        cozmo_vert_cube_get_is_visible: this.verticalCubeGetIsVisible,
        cozmo_vert_cube_get_position_2d: this.verticalCubeGetPosition2d,
        cozmo_vert_cube_get_position_3d: this.verticalCubeGetPosition3d,
        cozmo_vert_cube_get_pitch: this.verticalCubeGetPitch,
        cozmo_vert_cube_get_roll: this.verticalCubeGetRoll,
        cozmo_vert_cube_get_yaw: this.verticalCubeGetYaw,
        // Devices
        cozmo_vert_device_get_pitch: this.verticalDeviceGetPitch,
        cozmo_vert_device_get_roll: this.verticalDeviceGetRoll,
        cozmo_vert_device_get_yaw: this.verticalDeviceGetYaw
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

// Methods for creating default initialized equivalents for CozmoWorldState
// These allow testing without having C# attached, and ensure there's always 
// a default initialized state before the first update.
function InitVectorState(x, y, z) {
    // Return a json-equivalent vector2 or vector3
    var jsonData = {};
    jsonData.x = x;
    jsonData.y = y;
    if (z != null) {
        jsonData.z = z;
    }
    return jsonData;
}

function InitCubeState(cubeNum) {
    // Return a default initialized cube state
    var jsonData = {};
    jsonData.pos = InitVectorState(0.0, 0.0, 0.0);
    jsonData.camPos = InitVectorState(0.0, 0.0, null);
    jsonData.isValid = false;
    jsonData.isVisible = false;
    jsonData.wasJustTapped = false;
    jsonData.framesSinceTapped = 999999999;
    jsonData.pitch_d = 0.0;
    jsonData.roll_d = 0.0;
    jsonData.yaw_d = 0.0;
    return jsonData;
}

function InitFaceState() {
    // Return a default initialized face state
    var jsonData = {};
    jsonData.pos = InitVectorState(0.0, 0.0, 0.0);
    jsonData.camPos = InitVectorState(0.0, 0.0, null);
    jsonData.name = "";
    jsonData.isVisible = false;
    jsonData.expression = "unknown";
    return jsonData;
}

function InitDeviceState() {
    // Return a default initialized device state
    var jsonData = {};
    jsonData.pitch_d = 0.0;
    jsonData.roll_d = 0.0;
    jsonData.yaw_d = 0.0;
    return jsonData;
}

function InitTestCozmoWorldState() {
    // Return a default initialized cozmo world state
    var jsonData = {};
    jsonData.posePitch_d = 0.0;
    jsonData.poseRoll_d = 0.0;
    jsonData.poseYaw_d = 0.0;
    jsonData.liftHeightPercentage = 0.0;
    jsonData.headAngle_d = 0.0;
    jsonData.isPickedUp = false;
    jsonData.lastTappedCube = 0;
    jsonData.pos = InitVectorState(0.0, 0.0, 0.0);
    jsonData.cube1 = InitCubeState(1);
    jsonData.cube2 = InitCubeState(2);
    jsonData.cube3 = InitCubeState(3);
    jsonData.face = InitFaceState();
    jsonData.device = InitDeviceState();
    return jsonData;
}

// Global variable containing Cozmo's World's latest state (as a JSON object)
var gCozmoWorldState = InitTestCozmoWorldState();

window.setCozmoState = function(cozmoStateStr) {
    var cozmoState = JSON.parse(cozmoStateStr);
    gCozmoWorldState = cozmoState;
}

Scratch3CozmoBlocks.prototype.setBackpackColor = function(args, util) {
    // Cozmo performs this request instantly, so we have a timeout instead in js
    // so the user can see each backpack light setting.
    //
    // Pass -1 for requestId to indicate we don't need a Promise resolved.
    if (!util.stackFrame.timer) {
        var choiceString = Cast.toString(args.CHOICE);
        var colorHexValue = this._getColor(choiceString);
        window.Unity.call({requestId: -1, command: "cozmoSetBackpackColor", argString: choiceString, argUInt: colorHexValue});
        
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

Scratch3CozmoBlocks.prototype.verticalSetBackpackColor = function(args, util) {
    var colorHex = this._getColorIntFromColorObject(Cast.toRgbColorObject(args.COLOR));
    window.Unity.call({requestId: -1, command: "cozmoVerticalSetBackpackColor", argUInt: colorHex});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.driveForward = function(args, util) {
    return this.driveBlocksHelper(args, util, "cozmoDriveForward");
};

Scratch3CozmoBlocks.prototype.driveForwardFast = function(args, util) {
    return this.driveBlocksHelper(args, util, "cozmoDriveForwardFast");
};

Scratch3CozmoBlocks.prototype.driveBackward = function(args, util) {
    return this.driveBlocksHelper(args, util, "cozmoDriveBackward");
};

Scratch3CozmoBlocks.prototype.driveBackwardFast = function(args, util) {
    return this.driveBlocksHelper(args, util, "cozmoDriveBackwardFast");
};

Scratch3CozmoBlocks.prototype.driveBlocksHelper = function(args, util, command) {
    if (args.DISTANCE < 1) {
        return;
    }

    // The distMultiplier (as set by the parameter number under the block)
    // will be used as a multiplier against the base dist_mm.
    var distMultiplier = Cast.toNumber(args.DISTANCE);
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: command, argFloat: distMultiplier});

    return commandPromise;
};

Scratch3CozmoBlocks.prototype.playAnimationHelper = function(args, util, animIndex) {
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozmoPlayAnimation", argInt: animIndex});

    return commandPromise;
};

Scratch3CozmoBlocks.prototype.playAnimationHelperVertical = function(args, util, animIndex) {
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozmoPlayAnimation", argInt: animIndex});

    return commandPromise;
};

Scratch3CozmoBlocks.prototype.playAnimationFromDropdown = function(args, util) {
    var animIndex = Cast.toNumber(args.ANIMATION);
    return this.playAnimationHelperVertical(args, util, animIndex);
};

// ============================================================
// Begin temporary dev-only blocks prototyping
Scratch3CozmoBlocks.prototype.playNamedAnimationHelper = function(args, util, commandName, animName) {
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: commandName, argString: animName});

    return commandPromise;
};

Scratch3CozmoBlocks.prototype.playAnimationByName = function(args, util) {
    var animName = Cast.toString(args.ANIM_NAME);
    return this.playNamedAnimationHelper(args, util, "cozVertPlayNamedAnim", animName);
};

Scratch3CozmoBlocks.prototype.playAnimationByTriggerName = function(args, util) {    
    var triggerName = Cast.toString(args.TRIGGER_NAME);
    return this.playNamedAnimationHelper(args, util, "cozVertPlayNamedTriggerAnim", triggerName);
};
// End temporary dev-only blocks prototyping
// ============================================================

// See https://docs.google.com/spreadsheets/d/1wF0AjExf9p-yjOkt4LKdKsxIiO1KPxu5Knjv_q0EBQc/edit#gid=461013084
// for the animation mapping

Scratch3CozmoBlocks.prototype.playHappyAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 1); // "happy"
};

Scratch3CozmoBlocks.prototype.playVictoryAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 2); // "victory"
};

Scratch3CozmoBlocks.prototype.playUnhappyAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 3); // "unhappy"
};

Scratch3CozmoBlocks.prototype.playSurpriseAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 4); // "surprise"
};

Scratch3CozmoBlocks.prototype.playDogAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 5); // "dog"
};

Scratch3CozmoBlocks.prototype.playCatAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 6); // "cat"
};

Scratch3CozmoBlocks.prototype.playSneezeAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 7); // "sneeze"
};

Scratch3CozmoBlocks.prototype.playExcitedAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 8); // "excited"
};

Scratch3CozmoBlocks.prototype.playThinkingAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 9); // "thinking"
};

Scratch3CozmoBlocks.prototype.playBoredAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 10); // "bored"
};

Scratch3CozmoBlocks.prototype.playFrustratedAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 11); // "frustrated"
};

Scratch3CozmoBlocks.prototype.playChattyAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 12); // "chatty"
};

Scratch3CozmoBlocks.prototype.playDejectedAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 13); // "dejected"
};

Scratch3CozmoBlocks.prototype.playSleepAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 14); // "sleep"
};

Scratch3CozmoBlocks.prototype.playMysteryAnimation = function(args, util) {
    return this.playAnimationHelper(args, util, 0);
};

Scratch3CozmoBlocks.prototype.setLiftHeight = function(args, util) {
    var liftHeight = Cast.toString(args.CHOICE);
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozmoForklift", argString: liftHeight});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.setHeadAngle = function(args, util) {
    var headAngle = Cast.toString(args.CHOICE);
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozmoHeadAngle", argString: headAngle});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.dockWithCube = function(args, util) {
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozmoDockWithCube"});
    return commandPromise;
};
                                       
Scratch3CozmoBlocks.prototype.turnLeft = function(args, util) {
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozmoTurnLeft"});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.turnRight = function(args, util) {
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId , command: "cozmoTurnRight"});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.speak = function(args, util) {
    var textToSay = Cast.toString(args.STRING);
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozmoSays", argString: textToSay});
    return commandPromise;
};

/**
 * Wait until see face helper.
 *
 * @param argValues Parameters passed with the block.
 * @param util The util instance to use for yielding and finishing.
 * @param commandName The name for the specific wait-for-face command
 * @private
 */
Scratch3CozmoBlocks.prototype._waitUntilSeeFaceHelper = function (args, util, commandName) {
    // For now pass -1 for requestId to indicate we don't need a Promise resolved.
    window.Unity.call({requestId: -1, command: "cozmoHeadAngle", argString: "high"});

    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: commandName});
    return commandPromise;
};

/**
 * Wait until see any face.
 *
 * @param argValues Parameters passed with the block.
 * @param util The util instance to use for yielding and finishing.
 * @private
 */
Scratch3CozmoBlocks.prototype.waitUntilSeeFace = function (args, util) {
    return this._waitUntilSeeFaceHelper(args, util, "cozmoWaitUntilSeeFace");
};

/**
 * Wait until see happy face.
 *
 * @param argValues Parameters passed with the block.
 * @param util The util instance to use for yielding and finishing.
 * @private
 */
Scratch3CozmoBlocks.prototype.waitUntilSeeHappyFace = function (args, util) {
    return this._waitUntilSeeFaceHelper(args, util, "cozmoWaitUntilSeeHappyFace");
};

/**
 * Wait until see sad face.
 *
 * @param argValues Parameters passed with the block.
 * @param util The util instance to use for yielding and finishing.
 * @private
 */
Scratch3CozmoBlocks.prototype.waitUntilSeeSadFace = function (args, util) {
    return this._waitUntilSeeFaceHelper(args, util, "cozmoWaitUntilSeeSadFace");
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
    window.Unity.call({requestId: -1, command: "cozmoHeadAngle", argString: "low"});

    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozmoWaitUntilSeeCube"});
    return commandPromise;
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
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozmoWaitForCubeTap"});
    return commandPromise;
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
        {colorName: 'orange', colorHex: 0xff6600ff},
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

Scratch3CozmoBlocks.prototype._getColorIntFromColorObject = function(rgbColor) {
    // Color from rgb to hex value (like 0xffffffff).
    var colorHexValue = Color.rgbToHex(rgbColor);

    // Strip leading '#' char
    colorHexValue = colorHexValue.substring(1, colorHexValue.length);

    // Prepend "0x".
    colorHexValue = "0x" + colorHexValue;

    if (colorHexValue != "0x000000") {
        // Append alpha channel to all but black
        colorHexValue = colorHexValue + "ff";
    }

    // Convert from string to number
    return parseInt(colorHexValue);
};


// ================================================================================================================================================================
// Vertical Grammar
// ================================================================================================================================================================
// Actions:
// ========

Scratch3CozmoBlocks.prototype.verticalEnableAnimationTrack = function(args, util) {    
    var trackName = Cast.toString(args.ANIMATION_TRACK);
    window.Unity.call({requestId: -1, command: "cozVertEnableAnimationTrack", argString: trackName, argBool: true});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalDisableAnimationTrack = function(args, util) {    
    var trackName = Cast.toString(args.ANIMATION_TRACK);
    window.Unity.call({requestId: -1, command: "cozVertEnableAnimationTrack", argString: trackName, argBool: false});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalEnableWaitForActions = function(args, util) {    
    window.Unity.call({requestId: -1, command: "cozVertEnableWaitForActions", argBool: true});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalDisableWaitForActions = function(args, util) {    
    window.Unity.call({requestId: -1, command: "cozVertEnableWaitForActions", argBool: false});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalWaitForActions = function(args, util) {
    var requestId = this._getRequestId();
    var actionType = Cast.toString(args.ACTION_SELECT);
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozVertWaitForActions", argString: actionType});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.verticalCancelActions = function(args, util) {    
    var actionType = Cast.toString(args.ACTION_SELECT);
    window.Unity.call({requestId: -1, command: "cozVertCancelActions", argString: actionType});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalTurn = function(args, util) {
    var requestId = this._getRequestId();
    var turnAngle = Cast.toNumber(args.ANGLE);
    var speed = Cast.toNumber(args.SPEED);
    
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozVertTurn", argFloat: turnAngle, argFloat2: speed});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.verticalDrive = function(args, util) {
    var requestId = this._getRequestId();
    var distance = Cast.toNumber(args.DISTANCE);
    var speed = Cast.toNumber(args.SPEED);
    
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozVertDrive", argFloat: distance, argFloat2: speed});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.verticalDriveWheels = function(args, util) {
    var leftSpeed = Cast.toNumber(args.LEFT_SPEED);
    var rightSpeed = Cast.toNumber(args.RIGHT_SPEED);
    
    window.Unity.call({requestId: -1, command: "cozVertDriveWheels", argFloat: leftSpeed, argFloat2: rightSpeed});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalStopMotor = function(args, util) {
    var motorToStop = Cast.toString(args.MOTOR_SELECT);
    
    window.Unity.call({requestId: -1, command: "cozVertStopMotor", argString: motorToStop});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalPathOffset = function(args, util) {
    var requestId = this._getRequestId();
    var offsetX = Cast.toNumber(args.OFFSET_X);
    var offsetY = Cast.toNumber(args.OFFSET_Y);
    var offsetAngle = Cast.toNumber(args.OFFSET_ANGLE);
    
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozVertPathOffset", argFloat: offsetX, argFloat2: offsetY, argFloat3: offsetAngle});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.verticalPathTo = function(args, util) {
    var requestId = this._getRequestId();
    var newX = Cast.toNumber(args.NEW_X);
    var newY = Cast.toNumber(args.NEW_Y);
    var newAngle = Cast.toNumber(args.NEW_ANGLE);
    
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozVertPathTo", argFloat: newX, argFloat2: newY, argFloat3: newAngle});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.verticalSetHeadAngle = function(args, util) {
    var requestId = this._getRequestId();
    var angle = Cast.toNumber(args.HEAD_ANGLE);
    var speed = Cast.toNumber(args.SPEED);
    
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozVertHeadAngle", argFloat: angle, argFloat2: speed});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.verticalSetLiftHeight = function(args, util) {
    var requestId = this._getRequestId();
    var heightRatio = Cast.toNumber(args.HEIGHT_RATIO);
    var speed = Cast.toNumber(args.SPEED);

    var commandPromise = this._promiseForCommand(requestId);    
    window.Unity.call({requestId: requestId, command: "cozVertLiftHeight", argFloat: heightRatio, argFloat2: speed});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.verticalDockWithCubeById = function(args, util) {
    var requestId = this._getRequestId();
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var commandPromise = this._promiseForCommand(requestId);
    window.Unity.call({requestId: requestId, command: "cozVertDockWithCubeById", argUInt: cubeIndex});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.verticalMoveLift = function(args, util) {
    var speed = Cast.toNumber(args.LIFT_SPEED);

    window.Unity.call({requestId: -1, command: "cozVertMoveLift", argFloat: speed});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalSetCubeLightCorner = function(args, util) {
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var lightIndex = Cast.toNumber(args.LIGHT_SELECT);
    var colorHex = this._getColorIntFromColorObject(Cast.toRgbColorObject(args.COLOR));
    window.Unity.call({requestId: -1, command: "cozVertSetCubeLightCorner", argUInt: colorHex, argUInt2: cubeIndex, argUInt3: lightIndex});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalCubeAnim = function(args, util) {
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var cubeAnim = Cast.toString(args.ANIM_SELECT);
    var colorHex = this._getColorIntFromColorObject(Cast.toRgbColorObject(args.COLOR));
    window.Unity.call({requestId: -1, command: "cozVertCubeAnimation", argUInt: colorHex, argUInt2: cubeIndex, argString: cubeAnim});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalPlaySound = function(args, util) {
    var soundSelection = Cast.toNumber(args.SOUND_MENU);
    window.Unity.call({requestId: -1, command: "cozVertPlaySoundEffects", argInt: soundSelection, argBool: false});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalPlaySoundAndWait = function(args, util) {
    var soundSelection = Cast.toNumber(args.SOUND_MENU);
    var requestId = this._getRequestId();
    var commandPromise = this._promiseForCommand(requestId);      
    window.Unity.call({requestId: requestId, command: "cozVertPlaySoundEffects", argInt: soundSelection, argBool: true});
    return commandPromise;
};

Scratch3CozmoBlocks.prototype.verticalStopSound = function(args, util) {
    var soundSelection = Cast.toNumber(args.SOUND_MENU);
    window.Unity.call({requestId: -1, command: "cozVertStopSoundEffects", argInt: soundSelection});
    return window.Unity.sleepPromiseIfNecessary();
};

// Drawing on Cozmo's Face

Scratch3CozmoBlocks.prototype.verticalCozmoFaceClear = function(args, util) {
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceClear"});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalCozmoFaceDisplay = function(args, util) {
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceDisplay"});
    return window.Unity.sleepPromiseIfNecessary();
};   

Scratch3CozmoBlocks.prototype.verticalCozmoFaceSetDrawColor = function(args, util) {
    var drawColor = Cast.toBoolean(args.DRAW_COLOR);
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceSetDrawColor", argBool: drawColor});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalCozmoFaceSetTextScale = function(args, util) {
    var textScale = Cast.toNumber(args.SCALE);
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceSetTextScale", argFloat: textScale});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalCozmoFaceSetTextAlignment = function(args, util) {
    var xAlignment = Cast.toNumber(args.TEXT_ALIGNMENT_X); // 0,1,2 = left,center,right
    var yAlignment = Cast.toNumber(args.TEXT_ALIGNMENT_Y); // 0,1,2 = top,center,bottom
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceSetTextAlignment", argUInt: xAlignment, argUInt2: yAlignment});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalCozmoFaceDrawLine = function(args, util) {
    var x1 = Cast.toNumber(args.X1);
    var y1 = Cast.toNumber(args.Y1);
    var x2 = Cast.toNumber(args.X2);
    var y2 = Cast.toNumber(args.Y2);
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceDrawLine", argFloat: x1, argFloat2: y1, argFloat3: x2, argFloat4: y2});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalCozmoFaceFillRect = function(args, util) {
    var x1 = Cast.toNumber(args.X1);
    var y1 = Cast.toNumber(args.Y1);
    var x2 = Cast.toNumber(args.X2);
    var y2 = Cast.toNumber(args.Y2);
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceFillRect", argFloat: x1, argFloat2: y1, argFloat3: x2, argFloat4: y2});
    return window.Unity.sleepPromiseIfNecessary();
};
        
Scratch3CozmoBlocks.prototype.verticalCozmoFaceDrawRect = function(args, util) {
    var x1 = Cast.toNumber(args.X1);
    var y1 = Cast.toNumber(args.Y1);
    var x2 = Cast.toNumber(args.X2);
    var y2 = Cast.toNumber(args.Y2);
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceDrawRect", argFloat: x1, argFloat2: y1, argFloat3: x2, argFloat4: y2});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalCozmoFaceFillCircle = function(args, util) {
    var x1 = Cast.toNumber(args.X1);
    var y1 = Cast.toNumber(args.Y1);
    var radius = Cast.toNumber(args.RADIUS);
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceFillCircle", argFloat: x1, argFloat2: y1, argFloat3: radius});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalCozmoFaceDrawCircle = function(args, util) {
    var x1 = Cast.toNumber(args.X1);
    var y1 = Cast.toNumber(args.Y1);
    var radius = Cast.toNumber(args.RADIUS);
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceDrawCircle", argFloat: x1, argFloat2: y1, argFloat3: radius});
    return window.Unity.sleepPromiseIfNecessary();
};

Scratch3CozmoBlocks.prototype.verticalCozmoFaceDrawText = function(args, util) {
    var x1 = Cast.toNumber(args.X1);
    var y1 = Cast.toNumber(args.Y1);
    var text = Cast.toString(args.TEXT);
    window.Unity.call({requestId: -1, command: "cozVertCozmoFaceDrawText", argFloat: x1, argFloat2: y1, argString: text});
    return window.Unity.sleepPromiseIfNecessary();
};

// =================
// Sensors / Inputs:
// =================

// Helpers

function getVector2Axis(vector2d, axis) {
    switch (axis) {
        case 0:
            return Cast.toNumber(vector2d.x);
        case 1:
            return Cast.toNumber(vector2d.y);
        default:
            console.error("Invalid 2d axis '" + axis + "'");
            return 0.0;
    }
};

function getVector3Axis(vector3d, axis) {
    switch (axis) {
        case 0:
            return Cast.toNumber(vector3d.x);
        case 1:
            return Cast.toNumber(vector3d.y);
        case 2:
            return Cast.toNumber(vector3d.z);
        default:
            console.error("Invalid 3d axis '" + axis + "'");
            return 0.0;
    }
};

// Cozmo

Scratch3CozmoBlocks.prototype.verticalCozmoGetPosition = function(args, util) {
    var axis = Cast.toNumber(args.AXIS);
    return getVector3Axis(gCozmoWorldState.pos, axis);
};

Scratch3CozmoBlocks.prototype.verticalCozmoGetPitch = function(args, util) {
    return Cast.toNumber(gCozmoWorldState.posePitch_d);
};

Scratch3CozmoBlocks.prototype.verticalCozmoGetRoll = function(args, util) {
    return Cast.toNumber(gCozmoWorldState.poseRoll_d);
};

Scratch3CozmoBlocks.prototype.verticalCozmoGetYaw = function(args, util) {
    return Cast.toNumber(gCozmoWorldState.poseYaw_d);
};

Scratch3CozmoBlocks.prototype.verticalCozmoGetLiftHeight = function(args, util) {
    return Cast.toNumber(gCozmoWorldState.liftHeightPercentage);
};

Scratch3CozmoBlocks.prototype.verticalCozmoGetHeadAngle = function(args, util) {
    return Cast.toNumber(gCozmoWorldState.headAngle_d);
};

Scratch3CozmoBlocks.prototype.verticalCozmoGetIsPickedUp = function(args, util) {
    return Cast.toBoolean(gCozmoWorldState.isPickedUp);
};

// Faces

Scratch3CozmoBlocks.prototype.verticalFaceGetIsVisible = function(args, util) {
    return Cast.toBoolean(gCozmoWorldState.face.isVisible);
};

Scratch3CozmoBlocks.prototype.verticalFaceGetName = function(args, util) {
    return Cast.toString(gCozmoWorldState.face.name);
};

Scratch3CozmoBlocks.prototype.verticalFaceGet2d = function(args, util) {
    var axis = Cast.toNumber(args.AXIS);
    return getVector2Axis(gCozmoWorldState.face.camPos, axis);
};

Scratch3CozmoBlocks.prototype.verticalFaceGet3d = function(args, util) {
    var axis = Cast.toNumber(args.AXIS);
    return getVector3Axis(gCozmoWorldState.face.pos, axis);
};

Scratch3CozmoBlocks.prototype.verticalFaceGetExpression = function(args, util) {
    return Cast.toString(gCozmoWorldState.face.expression);
};

// Cubes

Scratch3CozmoBlocks.prototype.getCubeHelper = function(cubeIndex) {
    switch(cubeIndex) {
        case 1:
            return gCozmoWorldState.cube1;
        case 2:
            return gCozmoWorldState.cube2;
        case 3:
            return gCozmoWorldState.cube3;
        default:
            console.error("Invalid cubeIndex '" + cubeIndex + "'");
            return null;
    }
}

Scratch3CozmoBlocks.prototype.verticalCubeGetLastTapped = function(args, util) {
    return Cast.toNumber(gCozmoWorldState.lastTappedCube);
};

Scratch3CozmoBlocks.prototype.verticalCubeGetWasTapped = function(args, util) {
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var srcCube = this.getCubeHelper(cubeIndex);
    if (srcCube != null) {
        return Cast.toBoolean(srcCube.wasJustTapped);
    }
    else {
        return false;
    }
};

Scratch3CozmoBlocks.prototype.verticalCubeGetIsVisible = function(args, util) {
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var srcCube = this.getCubeHelper(cubeIndex);
    if (srcCube != null) {
        return Cast.toBoolean(srcCube.isVisible);
    }
    else {
        return false;
    }
};

Scratch3CozmoBlocks.prototype.verticalCubeGetPosition2d = function(args, util) {
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var axis = Cast.toNumber(args.AXIS);
    var srcCube = this.getCubeHelper(cubeIndex);
    if (srcCube != null) {
        return getVector2Axis(srcCube.camPos, axis);
    }
    else {
        return 0.0;
    }
};

Scratch3CozmoBlocks.prototype.verticalCubeGetPosition3d = function(args, util) {
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var axis = Cast.toNumber(args.AXIS);
    var srcCube = this.getCubeHelper(cubeIndex);
    if (srcCube != null) {
        return getVector3Axis(srcCube.pos, axis);
    }
    else {
        return 0.0;
    }
};

Scratch3CozmoBlocks.prototype.verticalCubeGetPitch = function(args, util) {
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var srcCube = this.getCubeHelper(cubeIndex);
    if (srcCube != null) {
        return Cast.toNumber(srcCube.pitch_d);
    }
    else {
        return 0.0;
    }
};

Scratch3CozmoBlocks.prototype.verticalCubeGetRoll = function(args, util) {
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var srcCube = this.getCubeHelper(cubeIndex);
    if (srcCube != null) {
        return Cast.toNumber(srcCube.roll_d);
    }
    else {
        return 0.0;
    }
};

Scratch3CozmoBlocks.prototype.verticalCubeGetYaw = function(args, util) {
    var cubeIndex = Cast.toNumber(args.CUBE_SELECT);
    var srcCube = this.getCubeHelper(cubeIndex);
    if (srcCube != null) {
        return Cast.toNumber(srcCube.yaw_d);
    }
    else {
        return 0.0;
    }
};

// Devices

Scratch3CozmoBlocks.prototype.verticalDeviceGetPitch = function(args, util) {
    return Cast.toNumber(gCozmoWorldState.device.pitch_d);
};

Scratch3CozmoBlocks.prototype.verticalDeviceGetRoll = function(args, util) {
    return Cast.toNumber(gCozmoWorldState.device.roll_d);
};

Scratch3CozmoBlocks.prototype.verticalDeviceGetYaw = function(args, util) {
    return Cast.toNumber(gCozmoWorldState.device.yaw_d);
};

module.exports = Scratch3CozmoBlocks;

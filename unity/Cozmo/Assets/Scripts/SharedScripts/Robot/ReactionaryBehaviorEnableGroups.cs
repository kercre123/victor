using UnityEngine;

public class ReactionaryBehaviorEnableGroups : MonoBehaviour {
  public static string kWakeupId = "wakeup";
  public static string kPauseManagerId = "pause_manager";
  public static string kMinigameId = "unity_minigame";
  public static string kSayTextSlideId = "say_text_slide";
  public static string kDroneModeDriveId = "drone_mode_driving";
  public static string kDroneModeIdleId = "drone_mode_idle";
  public static string kSpeedTapRoundEndId = "speed_tap_roundend";
  public static string kOnboardingUpdateStageId = "onboarding_update_stage";
  public static string kOnboardingBigReactionsOffId = "onboarding_big_reactions_off";

  // triggers disabled during wake up sequence.
  public static Anki.Cozmo.AllTriggersConsidered kWakeupTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    robotFalling: false,
    robotPickedUp: false,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: true,
    unexpectedMovement: true
  );

  // triggers that are disabled during challenges. some activities/games will choose to override this.
  public static Anki.Cozmo.AllTriggersConsidered kDefaultMinigameTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    robotFalling: false,
    robotPickedUp: false,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: true,
    unexpectedMovement: false
  );

  // in repair we also disable certain reactions when in severe repair state: (to be consistent with freeplay)
  //    robot on back, side, face, robotPickedUp, returnedToTreads
  public static Anki.Cozmo.AllTriggersConsidered kRepairGameSevereTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    robotFalling: true,
    robotPickedUp: true,
    robotPlacedOnSlope: true,
    returnedToTreads: true,
    robotOnBack: true,
    robotOnFace: true,
    robotOnSide: true,
    robotShaken: false,
    sparked: true,
    unexpectedMovement: false
  );

  // cube pounce overrides the default challenge trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kCubePounceGameTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    robotFalling: true,
    robotPickedUp: true,
    robotPlacedOnSlope: true,
    returnedToTreads: true,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: true,
    unexpectedMovement: true
  );

  // drone mode overrides the default challenge trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kDroneModeGameTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    robotFalling: false,
    robotPickedUp: false,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: true,
    sparked: true,
    unexpectedMovement: false
  );

  // face enrollment overrides the default challenge trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kFaceEnrollmentTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    facePositionUpdated: false,
    fistBump: true,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: false,
    petInitialDetection: true,
    robotFalling: false,
    robotPickedUp: false,
    robotPlacedOnSlope: false,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: true,
    sparked: false,
    unexpectedMovement: false
  );

  // memory match overrides the default challenge trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kMemoryMatchGameTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    robotFalling: true,
    robotPickedUp: true,
    robotPlacedOnSlope: true,
    returnedToTreads: true,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: true,
    unexpectedMovement: true
  );

  // cozmo says overrides the default challenge trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kSayTextSlideTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: false,
    facePositionUpdated: false,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: false,
    petInitialDetection: false,
    robotFalling: true,
    robotPickedUp: true,
    robotPlacedOnSlope: false,
    returnedToTreads: true,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    unexpectedMovement: true
  );

  // additional trigger disables in idle drive states for drone mode
  public static Anki.Cozmo.AllTriggersConsidered kDroneModeIdleTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    facePositionUpdated: true,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: false,
    robotFalling: false,
    robotPickedUp: false,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    unexpectedMovement: true
  );

  // additional trigger disables for drone mode when driving
  public static Anki.Cozmo.AllTriggersConsidered kDroneModeDriveTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    facePositionUpdated: true,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: false,
    robotFalling: true,
    robotPickedUp: true,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    unexpectedMovement: true
  );

  // special speed tap end of round trigger disables
  public static Anki.Cozmo.AllTriggersConsidered kSpeedTapRoundEndTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: false,
    facePositionUpdated: false,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: false,
    petInitialDetection: false,
    robotFalling: true,
    robotPickedUp: true,
    robotPlacedOnSlope: false,
    returnedToTreads: true,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    unexpectedMovement: false
  );

  public static Anki.Cozmo.AllTriggersConsidered kOnboardingBigReactionsOffTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    facePositionUpdated: false,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: true,
    petInitialDetection: true,
    robotFalling: false,
    robotPickedUp: false,
    robotPlacedOnSlope: false,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    unexpectedMovement: false
  );

  // triggers disabled while app is backgrounded
  public static Anki.Cozmo.AllTriggersConsidered kAppBackgroundedTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    facePositionUpdated: false,
    fistBump: false,
    frustration: false,
    hiccup: false,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: true,
    petInitialDetection: false,
    robotFalling: false,
    robotPickedUp: false,
    robotPlacedOnSlope: false,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    unexpectedMovement: false
  );

  // Reactions disabled when connected to a simulated robot because their sensors don't handle the trigger properly
  public static Anki.Cozmo.AllTriggersConsidered kSimulatedRobotTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    facePositionUpdated: false,
    fistBump: false,
    frustration: false,
    hiccup: false,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: false,
    petInitialDetection: false,
    robotFalling: false,
    robotPickedUp: false,
    robotPlacedOnSlope: false,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: true,
    sparked: false,
    unexpectedMovement: false
  );


}

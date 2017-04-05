using UnityEngine;

public class ReactionaryBehaviorEnableGroups : MonoBehaviour {
  public static string kWakeupId = "wakeup";
  public static string kPauseManagerId = "pause_manager";
  public static string kMinigameId = "unity_minigame";
  public static string kSayTextSlideId = "say_text_slide";
  public static string kDroneModeDriveId = "drone_mode_driving";
  public static string kDroneModeIdleId = "drone_mode_idle";
  public static string kSpeedTapRoundEndId = "speed_tap_roundend";
  public static string kOnboardingHomeId = "onboarding_home";
  public static string kOnboardingUpdateStageId = "onboarding_update_stage";
  public static string kOnboardingCubeStageId = "onboarding_cube_stage";

  // triggers disabled during wake up sequence.
  public static Anki.Cozmo.AllTriggersConsidered kWakeupTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: true,
    doubleTapDetected: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    pyramidInitialDetection: true,
    robotPickedUp: false,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: true,
    stackOfCubesInitialDetection: true,
    unexpectedMovement: true,
    vc: true
  );

  // triggers that are disabled during minigames. some activities/games will choose to override this.
  public static Anki.Cozmo.AllTriggersConsidered kDefaultMinigameTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: true,
    doubleTapDetected: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    pyramidInitialDetection: true,
    robotPickedUp: false,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: true,
    stackOfCubesInitialDetection: true,
    unexpectedMovement: false,
    vc: true
  );

  // cube pounce overrides the default minigame trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kCubePounceGameTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: true,
    doubleTapDetected: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    pyramidInitialDetection: true,
    robotPickedUp: true,
    robotPlacedOnSlope: true,
    returnedToTreads: true,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: true,
    stackOfCubesInitialDetection: true,
    unexpectedMovement: true,
    vc: true
  );

  // drone mode overrides the default minigame trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kDroneModeGameTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: true,
    doubleTapDetected: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    pyramidInitialDetection: true,
    robotPickedUp: false,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: true,
    sparked: true,
    stackOfCubesInitialDetection: true,
    unexpectedMovement: false,
    vc: true
  );

  // face enrollment overrides the default minigame trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kFaceEnrollmentTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    doubleTapDetected: false,
    facePositionUpdated: false,
    fistBump: true,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: false,
    petInitialDetection: true,
    pyramidInitialDetection: false,
    robotPickedUp: false,
    robotPlacedOnSlope: false,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    stackOfCubesInitialDetection: false,
    unexpectedMovement: false,
    vc: true
  );

  // memory match overrides the default minigame trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kMemoryMatchGameTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: true,
    doubleTapDetected: true,
    facePositionUpdated: true,
    fistBump: true,
    frustration: true,
    hiccup: true,
    motorCalibration: true,
    noPreDockPoses: true,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: true,
    pyramidInitialDetection: true,
    robotPickedUp: true,
    robotPlacedOnSlope: true,
    returnedToTreads: true,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: true,
    stackOfCubesInitialDetection: true,
    unexpectedMovement: true,
    vc: true
  );

  // cozmo says overrides the default minigame trigger disables with this
  public static Anki.Cozmo.AllTriggersConsidered kSayTextSlideTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: false,
    doubleTapDetected: false,
    facePositionUpdated: false,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: false,
    petInitialDetection: false,
    pyramidInitialDetection: false,
    robotPickedUp: true,
    robotPlacedOnSlope: false,
    returnedToTreads: true,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    stackOfCubesInitialDetection: false,
    unexpectedMovement: true,
    vc: true
  );

  // additional trigger disables in idle drive states for drone mode
  public static Anki.Cozmo.AllTriggersConsidered kDroneModeIdleTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    doubleTapDetected: false,
    facePositionUpdated: true,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: false,
    pyramidInitialDetection: false,
    robotPickedUp: false,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    stackOfCubesInitialDetection: false,
    unexpectedMovement: true,
    vc: true
  );

  // additional trigger disables for drone mode when driving
  public static Anki.Cozmo.AllTriggersConsidered kDroneModeDriveTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    doubleTapDetected: false,
    facePositionUpdated: true,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: true,
    placedOnCharger: false,
    petInitialDetection: false,
    pyramidInitialDetection: false,
    robotPickedUp: true,
    robotPlacedOnSlope: true,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    stackOfCubesInitialDetection: false,
    unexpectedMovement: true,
    vc: true
  );

  // special speed tap end of round trigger disables
  public static Anki.Cozmo.AllTriggersConsidered kSpeedTapRoundEndTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: true,
    cubeMoved: false,
    doubleTapDetected: false,
    facePositionUpdated: false,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: false,
    petInitialDetection: false,
    pyramidInitialDetection: false,
    robotPickedUp: true,
    robotPlacedOnSlope: false,
    returnedToTreads: true,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    stackOfCubesInitialDetection: false,
    unexpectedMovement: false,
    vc: true
  );

  public static Anki.Cozmo.AllTriggersConsidered kOnboardingHomeTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    doubleTapDetected: false,
    facePositionUpdated: false,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: true,
    petInitialDetection: false,
    pyramidInitialDetection: false,
    robotPickedUp: false,
    robotPlacedOnSlope: false,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    stackOfCubesInitialDetection: false,
    unexpectedMovement: false,
    vc: true
  );

  public static Anki.Cozmo.AllTriggersConsidered kOnboardingShowCubeStageTriggers = new Anki.Cozmo.AllTriggersConsidered(
    cliffDetected: false,
    cubeMoved: false,
    doubleTapDetected: false,
    facePositionUpdated: false,
    fistBump: false,
    frustration: false,
    hiccup: true,
    motorCalibration: false,
    noPreDockPoses: false,
    objectPositionUpdated: false,
    placedOnCharger: false,
    petInitialDetection: true,
    pyramidInitialDetection: false,
    robotPickedUp: false,
    robotPlacedOnSlope: false,
    returnedToTreads: false,
    robotOnBack: false,
    robotOnFace: false,
    robotOnSide: false,
    robotShaken: false,
    sparked: false,
    stackOfCubesInitialDetection: false,
    unexpectedMovement: false,
    vc: true
  );

}

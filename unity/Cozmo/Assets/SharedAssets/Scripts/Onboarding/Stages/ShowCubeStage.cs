using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Cozmo.UI;
using DataPersistence;

namespace Onboarding {
  public class ShowCubeStage : OnboardingBaseStage {

    [SerializeField]
    private AnkiTextLabel _ShowCozmoCubesLabel;

    [SerializeField]
    private AnkiTextLabel _FullScreenLabel;

    [SerializeField]
    private AnkiTextLabel _ShowShelfTextLabel;

    [SerializeField]
    private RectTransform _CozmoImageTransform;

    [SerializeField]
    private RectTransform _CozmoCubeRightSideUpTransform;

    [SerializeField]
    private RectTransform _CozmoMovedErrorTransform;

    [SerializeField]
    private RectTransform _ImageVisionCone;
    [SerializeField]
    private RectTransform _ImageCubeLights;

    [SerializeField]
    private CozmoButton _ContinueButtonInstance;

    private const int _kMaxPickupTries = 6;
    private const int _kMaxErrorsShown = 3;
    private const float _kMaxTimeInStage_Sec = 60 * 5;
    private const float _kBackupDistanceToCubeMM = 50.0f;


    private int _SawCubeID = -1;
    private int _CubePickupRetries = 0;
    private uint _PickupIDTag = 0;
    private int _ErrorsShown = 0;
    private float _StartTime;
    private int _CubesFoundTimes = 0;
    private bool _CubeShouldBeStill = false;


    enum SubState {
      ErrorCozmo,
      ErrorCubeMoved,
      ErrorCubeWrongSideUp,
      ErrorFinal,
      WaitForShowCube,
      WaitForOKCubeDiscovered,
      WaitForInspectCube,
      WaitForFinalContinue,
    };
    private SubState _SubState;

    public override void Start() {
      base.Start();
      _StartTime = Time.time;

      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
      RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      UpdateSubstate(SubState.WaitForShowCube);

      LightCube.OnMovedAction += HandleCubeMoved;
      RobotEngineManager.Instance.AddCallback<ReactionaryBehaviorTransition>(HandleRobotReactionaryBehavior);
      // Because we need very specific information about how something failed to give accurate error messages.
      // we need nitty gritty info from the engine not just a success bool.
      RobotEngineManager.Instance.AddCallback<RobotCompletedAction>(ProcessRobotCompletedAction);

      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe, Color.white);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Onboarding__Show_Cube);
      _StartTime = Time.time;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      LightCube.OnMovedAction -= HandleCubeMoved;
      RobotEngineManager.Instance.RemoveCallback<ReactionaryBehaviorTransition>(HandleRobotReactionaryBehavior);
      RobotEngineManager.Instance.RemoveCallback<RobotCompletedAction>(ProcessRobotCompletedAction);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Onboarding__Play_Tab);
    }

    private void HandleCubeMoved(int id, float accX, float accY, float aaZ) {
      if (_SawCubeID == id && _CubeShouldBeStill) {
        if (_SubState == SubState.WaitForInspectCube) {
          UpdateSubstate(SubState.ErrorCubeMoved);
        }
      }
    }

    protected void HandleRobotReactionaryBehavior(object messageObject) {
      ReactionaryBehaviorTransition behaviorTransition = messageObject as ReactionaryBehaviorTransition;
      if (behaviorTransition.behaviorStarted) {
        if (behaviorTransition.reactionaryBehaviorType == BehaviorType.ReactToCliff ||
            behaviorTransition.reactionaryBehaviorType == BehaviorType.ReactToPickup ||
            behaviorTransition.reactionaryBehaviorType == BehaviorType.ReactToRobotOnSide ||
            behaviorTransition.reactionaryBehaviorType == BehaviorType.ReactToRobotOnBack ||
            behaviorTransition.reactionaryBehaviorType == BehaviorType.ReactToRobotOnFace ||
            behaviorTransition.reactionaryBehaviorType == BehaviorType.ReactToUnexpectedMovement) {
          UpdateSubstate(SubState.ErrorCozmo);
        }
      }
    }

    protected void HandleContinueClicked() {
      if (_SubState == SubState.WaitForOKCubeDiscovered) {
        UpdateSubstate(SubState.WaitForInspectCube);
      }
      else if (_SubState == SubState.ErrorCozmo || _SubState == SubState.ErrorCubeMoved) {
        UpdateSubstate(SubState.WaitForShowCube);
      }
      else {
        // From SubState.WaitForFinalContinue or SubState.ErrorFinal
        OnboardingManager.Instance.GoToNextStage();
      }
    }

    private void HandleCubeReactComplete(bool success) {
      DAS.Debug(this, "HandleCubeReactComplete " + success);
      // Get to the right distance
      IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      if (CurrentRobot.LightCubes.ContainsKey(_SawCubeID)) {
        DAS.Debug(this, "HandleCubeReactComplete _SawCubeID" + _SawCubeID);
        LightCube block = CurrentRobot.LightCubes[_SawCubeID];
        _CubeShouldBeStill = false;
        // This will eventually call HandleCubePickupComplete via ProcessRobotCompletedAction
        _PickupIDTag = CurrentRobot.PickupObject(block);
      }
      else {
        UpdateSubstate(SubState.WaitForShowCube);
      }
    }

    private void ProcessRobotCompletedAction(RobotCompletedAction message) {
      uint idTag = message.idTag;
      if (_PickupIDTag == idTag) {
        HandleCubePickupComplete(message.result, message.completionInfo);
      }
    }

    private void HandleCubePickupComplete(ActionResult result, ActionCompletedUnion completionUnion) {
      bool success = result == ActionResult.SUCCESS;
      DAS.Debug(this, "HandleCubePickupComplete " + success);
      // If an error has happened the error UI will restart
      if (_SubState == SubState.WaitForInspectCube) {
        if (!success && _CubePickupRetries < _kMaxPickupTries) {
          bool wantsRetry = true;
          if (completionUnion.GetTag() == ActionCompletedUnion.Tag.objectInteractionCompleted) {
            DAS.Info(this, "HandleCubePickupComplete FAIL " + completionUnion.objectInteractionCompleted.result);
            if (completionUnion.objectInteractionCompleted.result == ObjectInteractionResult.VISUAL_VERIFICATION_FAILED) {
              wantsRetry = false;
            }
          }
          // We want to retry if the drive failed, but not if the pickup failed
          if (wantsRetry) {
            _CubePickupRetries++;
            // Try to pick up again...
            RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.RollBlockRealign, HandleCubeReactComplete);
          }
          else {
            UpdateSubstate(SubState.ErrorFinal);
          }
        }
        else {
          if (_CubePickupRetries >= _kMaxPickupTries) {
            UpdateSubstate(SubState.ErrorFinal);
          }
          else {
            RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingInteractWithCube,
                                                      HandlePickupAnimComplete, QueueActionPosition.NOW, false);
          }
        }
      }
    }

    private void HandlePickupAnimComplete(bool success) {
      // Put down cube, Back off...
      IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      DAS.Debug(this, "RobotStatusFlag.IS_CARRYING_BLOCK " + ((CurrentRobot.RobotStatus & RobotStatusFlag.IS_CARRYING_BLOCK) != 0) + " , " + CurrentRobot.RobotStatus);

      // There is a bug in the firmware where we have to make sure the lift is all the way up before Placing an object on ground.
      // And sometimes the animation doesn't make it all the way up
      CurrentRobot.SetLiftHeight(1.0f,
                                (bool success2) => { CurrentRobot.PlaceObjectOnGroundHere(HandlePutDownComplete); });

    }

    private void HandlePutDownComplete(bool success) {
      DAS.Debug(this, "HandlePutDownComplete " + success);
      RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(100.0f, -_kBackupDistanceToCubeMM, true, HandleBackupComplete);
    }

    private void HandleBackupComplete(bool success) {
      DAS.Debug(this, "HandleBackupComplete " + success);
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingReactToCubePutDown, HandleEndAnimationComplete);
    }
    private void HandleEndAnimationComplete(bool success) {
      DAS.Debug(this, "HandleEndAnimationComplete " + success);
      UpdateSubstate(SubState.WaitForFinalContinue);
    }

    public override void SkipPressed() {
      if (_SubState == SubState.WaitForFinalContinue) {
        OnboardingManager.Instance.GoToNextStage();
      }
      else {
        UpdateSubstate(SubState.WaitForFinalContinue);
      }
    }

    public void Update() {

      float timeInState = Time.time - _StartTime;
      // They're stuck in this state... just let them continue.
      if (timeInState > _kMaxTimeInStage_Sec &&
          _SubState != SubState.ErrorFinal &&
          _SubState != SubState.WaitForFinalContinue) {
        UpdateSubstate(SubState.ErrorFinal);
      }
      // Not using RobotObservedObject because we probably want to constantly track this and go to error if it changes back.
      if (_SubState == SubState.WaitForShowCube || _SubState == SubState.ErrorCozmo) {
        bool isAnyInView = false;
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
            isAnyInView |= kvp.Value.IsInFieldOfView;
            if (kvp.Value.IsInFieldOfView) {
              _SawCubeID = kvp.Key;
            }
          }
        }
        if (isAnyInView) {
          UpdateSubstate(SubState.WaitForOKCubeDiscovered);
        }
      }
      // TODO: uncomment once firmware fix gets in... COZMO-4230
      /*else if (_SubState == SubState.WaitForOKCubeDiscovered) {

        IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
        if (CurrentRobot.LightCubes.ContainsKey(_SawCubeID)) {
          LightCube block = CurrentRobot.LightCubes[_SawCubeID];
          if (block.UpAxis != UpAxis.ZPositive ) {
            UpdateSubstate(SubState.ErrorCubeWrongSideUp);
          }
        }
      }*/
      else if (_SubState == SubState.ErrorCubeWrongSideUp) {
        IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
        if (CurrentRobot.LightCubes.ContainsKey(_SawCubeID)) {
          LightCube block = CurrentRobot.LightCubes[_SawCubeID];
          if (block.UpAxis == UpAxis.ZPositive && block.IsInFieldOfView) {
            UpdateSubstate(SubState.WaitForInspectCube);
          }
        }
        else {
          UpdateSubstate(SubState.WaitForShowCube);
        }
      }
    }

    private void UpdateSubstate(SubState nextState) {
      if (nextState == SubState.WaitForShowCube) {
        _ShowShelfTextLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body1);
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Header1);
        _ContinueButtonInstance.gameObject.SetActive(false);
        _CozmoMovedErrorTransform.gameObject.SetActive(false);
        _ImageCubeLights.gameObject.SetActive(false);
        _ImageVisionCone.gameObject.SetActive(true);
      }
      else if (nextState == SubState.WaitForOKCubeDiscovered) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body2);
        _ShowShelfTextLabel.text = "";
        _ContinueButtonInstance.gameObject.SetActive(true);
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Block_Connect);
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingDiscoverCube);
        _ImageCubeLights.gameObject.SetActive(true);
        _ImageVisionCone.gameObject.SetActive(true);
        if (_CubesFoundTimes == 0) {
          float timeToFindCube = Time.time - _StartTime;
          DAS.Event("onboarding.upgrade", timeToFindCube.ToString());
        }
        _CubesFoundTimes++;
      }
      else if (nextState == SubState.WaitForInspectCube) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body3);
        _ShowShelfTextLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Header3);
        _ContinueButtonInstance.gameObject.SetActive(false);
        _CozmoCubeRightSideUpTransform.gameObject.SetActive(false);
        _CozmoImageTransform.gameObject.SetActive(true);
        _ImageCubeLights.gameObject.SetActive(true);
        _ImageVisionCone.gameObject.SetActive(false);
        // Get to the right distance
        IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
        if (CurrentRobot.LightCubes.ContainsKey(_SawCubeID)) {
          DAS.Debug(this, "AligningWithBlock: " + _SawCubeID);
          LightCube block = CurrentRobot.LightCubes[_SawCubeID];
          _CubeShouldBeStill = true;
          if (block.IsInFieldOfView) {
            RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingReactToCube, HandleCubeReactComplete);
          }
          else {
            UpdateSubstate(SubState.ErrorCubeMoved);
          }
        }
        else {
          UpdateSubstate(SubState.WaitForShowCube);
        }
      }
      else if (nextState == SubState.WaitForFinalContinue) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body4);
        _ShowShelfTextLabel.text = "";
        _ContinueButtonInstance.gameObject.SetActive(true);
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Attention_Device);
      }
      else if (nextState == SubState.ErrorCubeWrongSideUp) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCubeRightSideUp);
        _ShowShelfTextLabel.text = "";
        _ContinueButtonInstance.gameObject.SetActive(false);
        _CozmoCubeRightSideUpTransform.gameObject.SetActive(true);
        _CozmoImageTransform.gameObject.SetActive(false);
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Attention_Device);
        DAS.Event("onboarding.error", "error_cube_lights_up");
      }
      else if (nextState == SubState.ErrorCubeMoved || nextState == SubState.ErrorCozmo) {
        _ErrorsShown++;
        // They are messing with Cozmo too much, just move on...
        if (_ErrorsShown > _kMaxErrorsShown) {
          UpdateSubstate(SubState.ErrorFinal);
        }
        else {
          if (nextState == SubState.ErrorCubeMoved) {
            _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCube);
            _ShowShelfTextLabel.text = "";
            _ContinueButtonInstance.gameObject.SetActive(true);
            Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Attention_Device);
            DAS.Event("onboarding.error", "error_cube_moved");
          }
          else if (nextState == SubState.ErrorCozmo) {
            _ErrorsShown++;
            _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCozmo);
            _ShowShelfTextLabel.text = "";
            _CozmoImageTransform.gameObject.SetActive(false);
            _ContinueButtonInstance.gameObject.SetActive(true);
            _CozmoMovedErrorTransform.gameObject.SetActive(true);
            Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Attention_Device);
            DAS.Event("onboarding.error", "error_cozmo_moved");
          }
        }
      }
      else if (nextState == SubState.ErrorFinal) {
        _FullScreenLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorFinal);
        _ShowCozmoCubesLabel.text = "";
        _ShowShelfTextLabel.text = "";
        _ContinueButtonInstance.gameObject.SetActive(true);
        _CozmoMovedErrorTransform.gameObject.SetActive(false);
        _CozmoCubeRightSideUpTransform.gameObject.SetActive(false);
        _CozmoImageTransform.gameObject.SetActive(false);
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Attention_Device);
        DAS.Event("onboarding.error", "error_final");
      }
      _SubState = nextState;
    }
  }
}

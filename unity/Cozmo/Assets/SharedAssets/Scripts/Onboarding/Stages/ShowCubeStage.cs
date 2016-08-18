using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Cozmo.UI;
using DG.Tweening;
using DataPersistence;

namespace Onboarding {
  public class ShowCubeStage : OnboardingBaseStage {

    [SerializeField]
    private AnkiTextLabel _ShowCozmoCubesLabel;

    [SerializeField]
    private RectTransform _CozmoImageTransform;

    [SerializeField]
    private RectTransform _CozmoCubeRightSideUpTransform;

    [SerializeField]
    private CozmoButton _ContinueButtonInstance;

    private int _SawCubeID = -1;

    enum SubState {
      ErrorCozmo,
      ErrorCubeMoved,
      ErrorCubeWrongSideUp,
      WaitForShowCube,
      WaitForOKCubeDiscovered,
      WaitForInspectCube,
      WaitForFinalContinue,
    };
    private SubState _SubState;

    public override void Start() {
      base.Start();

      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
      RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      UpdateSubstate(SubState.WaitForShowCube);

      LightCube.OnMovedAction += HandleCubeMoved;
      RobotEngineManager.Instance.AddCallback<ReactionaryBehaviorTransition>(HandleRobotReactionaryBehavior);
    }
    public override void OnDestroy() {
      base.OnDestroy();
      LightCube.OnMovedAction -= HandleCubeMoved;
      RobotEngineManager.Instance.RemoveCallback<ReactionaryBehaviorTransition>(HandleRobotReactionaryBehavior);
    }

    private void HandleCubeMoved(int id, float accX, float accY, float aaZ) {
      // TODO: cubed move state needs a more specific warning.
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
      else if (_SubState == SubState.ErrorCozmo) {
        UpdateSubstate(SubState.WaitForShowCube);
      }
      else {
        // From SubState.WaitForFinalContinue
        OnboardingManager.Instance.GoToNextStage();
      }
    }

    private void HandleBlockAlignComplete(bool success) {
      DAS.Debug(this, "HandleBlockAlignComplete " + success);
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingReactToCube, HandleCubeReactComplete);
    }
    private void HandleCubeReactComplete(bool success) {
      DAS.Debug(this, "HandleCubeReactComplete " + success);
      // Get to the right distance
      IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      if (CurrentRobot.LightCubes.ContainsKey(_SawCubeID)) {
        DAS.Debug(this, "HandleCubeReactComplete _SawCubeID" + _SawCubeID);
        LightCube block = CurrentRobot.LightCubes[_SawCubeID];
        CurrentRobot.PickupObject(block, true, false, false, 0, HandleCubePickupComplete);
      }
      else {
        UpdateSubstate(SubState.WaitForShowCube);
      }
    }
    private void HandleCubePickupComplete(bool success) {
      DAS.Debug(this, "HandleCubePickupComplete " + success);
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingInteractWithCube, HandlePickupAnimComplete);
    }
    private void HandlePickupAnimComplete(bool success) {
      // Put down cube, Back off...
      IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      CurrentRobot.PlaceObjectOnGroundHere((bool success2) => { CurrentRobot.DriveStraightAction(100.0f, -20.0f, true, HandleBackupComplete); });
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
      else if (_SubState == SubState.WaitForOKCubeDiscovered) {
        IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
        if (CurrentRobot.LightCubes.ContainsKey(_SawCubeID)) {
          LightCube block = CurrentRobot.LightCubes[_SawCubeID];

          // TODO: remove unknown once firmware fix gets in... COZMO-3962
          if (block.UpAxis != UpAxis.ZPositive && block.UpAxis != UpAxis.Unknown) {
            UpdateSubstate(SubState.ErrorCubeWrongSideUp);
          }
        }
      }
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
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body1);
        _ContinueButtonInstance.gameObject.SetActive(false);
      }
      else if (nextState == SubState.WaitForOKCubeDiscovered) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body2);
        _ContinueButtonInstance.gameObject.SetActive(true);
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingDiscoverCube);
      }
      else if (nextState == SubState.WaitForInspectCube) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body3);
        _ContinueButtonInstance.gameObject.SetActive(false);
        _CozmoCubeRightSideUpTransform.gameObject.SetActive(false);
        _CozmoImageTransform.gameObject.SetActive(true);

        // Get to the right distance
        IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
        if (CurrentRobot.LightCubes.ContainsKey(_SawCubeID)) {
          DAS.Debug(this, "AligningWithBlock: " + _SawCubeID);
          LightCube block = CurrentRobot.LightCubes[_SawCubeID];
          CurrentRobot.AlignWithObject(block, 23.0f, HandleBlockAlignComplete, false, false);
        }
        else {
          UpdateSubstate(SubState.WaitForShowCube);
        }
      }
      else if (nextState == SubState.WaitForFinalContinue) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body4);
        _ContinueButtonInstance.gameObject.SetActive(true);
      }
      else if (nextState == SubState.ErrorCubeWrongSideUp) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCubeRightSideUp);
        _ContinueButtonInstance.gameObject.SetActive(false);
        _CozmoCubeRightSideUpTransform.gameObject.SetActive(true);
        _CozmoImageTransform.gameObject.SetActive(false);
      }
      else if (nextState == SubState.ErrorCubeMoved) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCube);
        _ContinueButtonInstance.gameObject.SetActive(false);
      }
      else if (nextState == SubState.ErrorCozmo) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCozmo);
        _ContinueButtonInstance.gameObject.SetActive(true);
      }
      _SubState = nextState;
    }
  }
}
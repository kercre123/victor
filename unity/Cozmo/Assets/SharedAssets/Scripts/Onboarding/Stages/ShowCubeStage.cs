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
    private CozmoButton _ContinueButtonInstance;

    private int _SawCubeID = -1;

    enum SubState {
      ErrorCozmo,
      ErrorCube,
      WaitForShowCube,
      WaitForEnergy,
      WaitForInspectCube,
      WaitForReactToCube,
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
      if (_SubState == SubState.WaitForEnergy) {
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

    private void HandleEndAnimationComplete(bool success) {
      UpdateSubstate(SubState.WaitForFinalContinue);
    }
    private void HandleBlockAlignComplete(bool success) {
      UpdateSubstate(SubState.WaitForReactToCube);
    }

    public override void SkipPressed() {
      if (_SubState == SubState.WaitForFinalContinue) {
        OnboardingManager.Instance.GoToNextStage();
      }
      else {
        UpdateSubstate((SubState)((int)_SubState + 1));
      }
    }

    public void Update() {
      // Not using RobotObservedObject because we probably want to constantly track this and go to error if it changes back.
      if (_SubState == SubState.WaitForShowCube || _SubState == SubState.ErrorCozmo) {
        bool isAnyInView = false;
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
            isAnyInView |= kvp.Value.IsInFieldOfView;
            _SawCubeID = kvp.Value.ID;
          }
        }
        if (isAnyInView) {
          UpdateSubstate(SubState.WaitForEnergy);
        }
      }
    }

    private void UpdateSubstate(SubState nextState) {
      if (nextState == SubState.WaitForShowCube) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body1);
        _ContinueButtonInstance.gameObject.SetActive(false);
      }
      else if (nextState == SubState.WaitForEnergy) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body2);
        _ContinueButtonInstance.gameObject.SetActive(true);
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnboardingReactToCube);
      }
      else if (nextState == SubState.WaitForInspectCube) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body3);
        _ContinueButtonInstance.gameObject.SetActive(false);

        // Get to the right distance
        IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
        if (CurrentRobot.LightCubes.ContainsKey(_SawCubeID)) {
          LightCube block = CurrentRobot.LightCubes[_SawCubeID];
          CurrentRobot.AlignWithObject(block, 20.0f, HandleBlockAlignComplete);
        }
        else {
          UpdateSubstate(SubState.WaitForReactToCube);
        }
      }
      else if (nextState == SubState.WaitForReactToCube) {
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnboardingInteractWithCube, HandleEndAnimationComplete);
      }
      else if (nextState == SubState.WaitForFinalContinue) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body4);
        _ContinueButtonInstance.gameObject.SetActive(true);
      }
      else if (nextState == SubState.ErrorCube) {
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
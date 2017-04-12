using UnityEngine;
using Anki.UI;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Cozmo.UI;

namespace Onboarding {
  public class ShowCubeStage : OnboardingBaseStage {

    [SerializeField]
    private AnkiTextLegacy _ShowCozmoCubesLabel;

    [SerializeField]
    private AnkiTextLegacy _FullScreenLabel;

    [SerializeField]
    private AnkiTextLegacy _ShowShelfTextLabel;

    [SerializeField]
    private RectTransform _CozmoImageTransform;

    [SerializeField]
    private RectTransform _CozmoCubeRightSideUpTransform;

    [SerializeField]
    private RectTransform _CozmoMovedErrorTransform;
    [SerializeField]
    private RectTransform _ImageCubeLights;

    [SerializeField]
    private CozmoButtonLegacy _ContinueButtonInstance;

    private int _CubesFoundTimes = 0;
    private float _StartTime;
    private OnboardingStateEnum _State = OnboardingStateEnum.Inactive;
    private BehaviorClass _CurrBehavior = BehaviorClass.NoneBehavior;

    private void Awake() {
      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
    }

    public override void Start() {
      base.Start();
      _StartTime = Time.time;

      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayLightStates(true);

      RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      RobotEngineManager.Instance.AddCallback<OnboardingState>(HandleUpdateOnboardingState);
      RobotEngineManager.Instance.AddCallback<BehaviorTransition>(HandleBehaviorTransition);

      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe, Color.white);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Onboarding__Show_Cube);

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.ExecuteBehaviorByName("OnboardingShowCube");
        // In this behavior we allow some interuptions, but ReactToPet can interrupt from those reactions. Repress it here, instead of the C++ behavior
        // so it will be off throughout the whole thing.
        RobotEngineManager.Instance.CurrentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kOnboardingCubeStageId, ReactionaryBehaviorEnableGroups.kOnboardingShowCubeStageTriggers);
      }
      // So there is no UI pop just start us at the right stage.
      // Next tick engine will set up at this state too.
      UpdateStateUI(OnboardingStateEnum.WaitForShowCube);
    }

    public override void OnDestroy() {
      base.OnDestroy();
      RobotEngineManager.Instance.RemoveCallback<OnboardingState>(HandleUpdateOnboardingState);
      RobotEngineManager.Instance.RemoveCallback<BehaviorTransition>(HandleBehaviorTransition);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Onboarding__Play_Tab);

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.ExecuteBehaviorByName("NoneBehavior");
        // re-enable reactionary behaviors.
        RobotEngineManager.Instance.CurrentRobot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kOnboardingCubeStageId);
      }
    }

    private void HandleBehaviorTransition(BehaviorTransition msg) {
      _CurrBehavior = msg.newBehaviorClass;

      if (_State == OnboardingStateEnum.ErrorCozmo) {
        // We're returning to this behavior.
        _ContinueButtonInstance.Interactable = _CurrBehavior == BehaviorClass.OnboardingShowCube;
      }
    }

    protected void HandleContinueClicked() {
      // If we're in the final error state, then we might have a defective robot that is always doing a reaction
      // let them continue with just the UI instead of waiting for the engine behavior to send back confirmation
      // since reactions are preventing the behavior from even running consistently.
      if (_State == OnboardingStateEnum.ErrorFinal &&
        _CurrBehavior != BehaviorClass.OnboardingShowCube) {
        OnboardingManager.Instance.GoToNextStage();
      }
      else {
        // Otherwise Engine is in complete control of the next state.
        RobotEngineManager.Instance.Message.TransitionToNextOnboardingState = Singleton<TransitionToNextOnboardingState>.Instance;
        RobotEngineManager.Instance.SendMessage();
      }
    }

    private void HandleUpdateOnboardingState(object messageObject) {
      OnboardingState msg = messageObject as OnboardingState;
      UpdateStateUI(msg.stateNum);
    }

    private void UpdateStateUI(OnboardingStateEnum nextState) {
      // Previous state cleanup
      if (_State == OnboardingStateEnum.ErrorCozmo) {
        RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      }
      // Setup UI for next state
      switch (nextState) {
      case OnboardingStateEnum.WaitForShowCube: {
          _ShowShelfTextLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body1);
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Header1);
          _ContinueButtonInstance.gameObject.SetActive(false);
          _CozmoMovedErrorTransform.gameObject.SetActive(false);
          _CozmoCubeRightSideUpTransform.gameObject.SetActive(false);
          _CozmoImageTransform.gameObject.SetActive(true);
          _ImageCubeLights.gameObject.SetActive(false);
        }
        break;
      case OnboardingStateEnum.WaitForOKCubeDiscovered: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body2);
          _ShowShelfTextLabel.text = "";
          _ContinueButtonInstance.gameObject.SetActive(true);
          _ContinueButtonInstance.Interactable = true;
          // Wait for the animation to be done before playing the context switch sound.
          RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingDiscoverCube,
                    (bool success) => {
                      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Block_Connect);
                    });
          _ImageCubeLights.gameObject.SetActive(true);
          if (_CubesFoundTimes == 0) {
            float timeToFindCube = Time.time - _StartTime;
            DAS.Event("onboarding.show_cube", timeToFindCube.ToString());
          }
          _CubesFoundTimes++;
        }
        break;
      case OnboardingStateEnum.WaitForInspectCube: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body3);
          _ShowShelfTextLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Header3);
          _ContinueButtonInstance.gameObject.SetActive(false);
          _CozmoCubeRightSideUpTransform.gameObject.SetActive(false);
          _CozmoImageTransform.gameObject.SetActive(true);
          _ImageCubeLights.gameObject.SetActive(true);
        }
        break;
      case OnboardingStateEnum.WaitForFinalContinue: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body4);
          _ShowShelfTextLabel.text = "";
          _CozmoMovedErrorTransform.gameObject.SetActive(false);
          _CozmoImageTransform.gameObject.SetActive(true);
          _ContinueButtonInstance.gameObject.SetActive(true);
          _ContinueButtonInstance.Interactable = true;
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.AudioMetaData.GameEvent.Ui.Attention_Device);
        }
        break;
      case OnboardingStateEnum.ErrorCubeWrongSideUp: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCubeRightSideUp);
          _ShowShelfTextLabel.text = "";
          _ContinueButtonInstance.gameObject.SetActive(false);
          _CozmoCubeRightSideUpTransform.gameObject.SetActive(true);
          _CozmoImageTransform.gameObject.SetActive(false);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.AudioMetaData.GameEvent.Ui.Attention_Device);
          DAS.Event("onboarding.error", "error_cube_lights_up");
        }
        break;
      case OnboardingStateEnum.ErrorCubeMoved: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCube);
          _ShowShelfTextLabel.text = "";
          _ContinueButtonInstance.gameObject.SetActive(false);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.AudioMetaData.GameEvent.Ui.Attention_Device);
          DAS.Event("onboarding.error", "error_cube_moved");
        }
        break;
      case OnboardingStateEnum.ErrorCozmo: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCozmo);
          _ShowShelfTextLabel.text = "";
          _CozmoCubeRightSideUpTransform.gameObject.SetActive(false);
          _CozmoImageTransform.gameObject.SetActive(false);
          _ContinueButtonInstance.gameObject.SetActive(true);
          // Becomes interactable again when we are done with the reactionary behavior.
          _ContinueButtonInstance.Interactable = false;
          _CozmoMovedErrorTransform.gameObject.SetActive(true);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.AudioMetaData.GameEvent.Ui.Attention_Device);
          DAS.Event("onboarding.error", "error_cozmo_moved");
        }
        break;
      case OnboardingStateEnum.ErrorFinal: {
          _FullScreenLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorFinal);
          _ShowCozmoCubesLabel.text = "";
          _ShowShelfTextLabel.text = "";
          _ContinueButtonInstance.gameObject.SetActive(true);
          _ContinueButtonInstance.Interactable = true;
          _CozmoMovedErrorTransform.gameObject.SetActive(false);
          _CozmoCubeRightSideUpTransform.gameObject.SetActive(false);
          _CozmoImageTransform.gameObject.SetActive(false);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.AudioMetaData.GameEvent.Ui.Attention_Device);
          DAS.Event("onboarding.error", "error_final");
        }
        break;
      case OnboardingStateEnum.Inactive: {
          OnboardingManager.Instance.GoToNextStage();
        }
        break;
      default:
        DAS.Warn("onboarding.unhandledcase", nextState.ToString());
        break;
      }
      DAS.Info("DEV onboarding statechange", "prev: " + _State + " next: " + nextState);
      _State = nextState;
    }
  }
}

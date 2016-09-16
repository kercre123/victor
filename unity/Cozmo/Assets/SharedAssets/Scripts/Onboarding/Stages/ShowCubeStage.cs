using UnityEngine;
using Anki.UI;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Cozmo.UI;

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

    private int _CubesFoundTimes = 0;
    private float _StartTime;
    private OnboardingStateEnum _State = OnboardingStateEnum.Inactive;

    public override void Start() {
      base.Start();
      _StartTime = Time.time;

      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");

      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayLightStates(true);

      RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      RobotEngineManager.Instance.AddCallback<OnboardingState>(HandleUpdateOnboardingState);

      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe, Color.white);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Onboarding__Show_Cube);

      RobotEngineManager.Instance.CurrentRobot.ExecuteBehaviorByName("OnboardingShowCube");
      // So there is no UI pop just start us at the right stage.
      // Next tick engine will set up at this state too.
      UpdateStateUI(OnboardingStateEnum.WaitForShowCube);

    }

    public override void OnDestroy() {
      base.OnDestroy();
      RobotEngineManager.Instance.RemoveCallback<OnboardingState>(HandleUpdateOnboardingState);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Onboarding__Play_Tab);

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.ExecuteBehaviorByName("NoneBehavior");
      }
    }

    protected void HandleContinueClicked() {
      RobotEngineManager.Instance.Message.TransitionToNextOnboardingState = Singleton<TransitionToNextOnboardingState>.Instance;
      RobotEngineManager.Instance.SendMessage();
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
          // Wait for the animation to be done before playing the context switch sound.
          RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingDiscoverCube,
                    (bool success) => {
                      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Block_Connect);
                    });
          _ImageCubeLights.gameObject.SetActive(true);
          if (_CubesFoundTimes == 0) {
            float timeToFindCube = Time.time - _StartTime;
            DAS.Event("onboarding.upgrade", timeToFindCube.ToString());
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
          _ImageVisionCone.gameObject.SetActive(true);
        }
        break;
      case OnboardingStateEnum.WaitForFinalContinue: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body4);
          _ShowShelfTextLabel.text = "";
          _ContinueButtonInstance.gameObject.SetActive(true);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Attention_Device);
        }
        break;
      case OnboardingStateEnum.ErrorCubeWrongSideUp: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCubeRightSideUp);
          _ShowShelfTextLabel.text = "";
          _ContinueButtonInstance.gameObject.SetActive(false);
          _CozmoCubeRightSideUpTransform.gameObject.SetActive(true);
          _CozmoImageTransform.gameObject.SetActive(false);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Attention_Device);
          DAS.Event("onboarding.error", "error_cube_lights_up");
        }
        break;
      case OnboardingStateEnum.ErrorCubeMoved: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCube);
          _ShowShelfTextLabel.text = "";
          _ContinueButtonInstance.gameObject.SetActive(true);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Attention_Device);
          DAS.Event("onboarding.error", "error_cube_moved");
        }
        break;
      case OnboardingStateEnum.ErrorCozmo: {
          _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorCozmo);
          _ShowShelfTextLabel.text = "";
          _CozmoImageTransform.gameObject.SetActive(false);
          _ContinueButtonInstance.gameObject.SetActive(true);
          _CozmoMovedErrorTransform.gameObject.SetActive(true);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Attention_Device);
          DAS.Event("onboarding.error", "error_cozmo_moved");
        }
        break;
      case OnboardingStateEnum.ErrorFinal: {
          _FullScreenLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3ErrorFinal);
          _ShowCozmoCubesLabel.text = "";
          _ShowShelfTextLabel.text = "";
          _ContinueButtonInstance.gameObject.SetActive(true);
          _CozmoMovedErrorTransform.gameObject.SetActive(false);
          _CozmoCubeRightSideUpTransform.gameObject.SetActive(false);
          _CozmoImageTransform.gameObject.SetActive(false);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Attention_Device);
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
      DAS.Info("onboarding.statechange", "prev: " + _State + " next: " + nextState);
      _State = nextState;
    }
  }
}

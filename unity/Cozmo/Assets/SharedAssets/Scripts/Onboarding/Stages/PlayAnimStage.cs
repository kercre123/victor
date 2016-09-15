using Cozmo.UI;
using UnityEngine;
using Anki.Cozmo;

namespace Onboarding {

  public class PlayAnimStage : OnboardingBaseStage {

    [SerializeField]
    private SerializableAnimationTrigger _Animation;

    [SerializeField]
    private Transform _Logo;

    [SerializeField]
    private Transform _OldRobotLogoPos;

    private IRobot _CurrentRobot;
    public override void Start() {
      base.Start();
      _CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Onboarding);
      _CurrentRobot.SendAnimationTrigger(_Animation.Value, HandleEndAnimationComplete);
      // Match the logo jumping of the previous screen
      bool isOldRobot = UnlockablesManager.Instance.IsUnlocked(Anki.Cozmo.UnlockId.StackTwoCubes);
      if (isOldRobot) {
        _Logo.position = _OldRobotLogoPos.position;
      }
    }

    public override void OnDestroy() {
      base.OnDestroy();
      Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Attention_Device);
    }

    public override void SkipPressed() {
      _CurrentRobot.CancelAction(RobotActionType.PLAY_ANIMATION);
      OnboardingManager.Instance.GoToNextStage();
    }

    private void HandleEndAnimationComplete(bool success) {
      // If we saw a face recently, look at it and play the "hello player"
      // Otherwise just play the "hello world" and be impressed with the enviornment.
      if (_CurrentRobot.Faces.Count > 0) {
        // In order to move things along quickly people don't want to wait for the vision system to confirm a
        // TurnTowardsFacePose, They've only had a few seconds since cozmo looked up so mostly should be in same place.
        _CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingHelloPlayer, HandleReactionEndAnimationComplete);
      }
      else {
        _CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingHelloWorld, HandleReactionEndAnimationComplete);
      }
    }

    private void HandleReactionEndAnimationComplete(bool success) {
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}

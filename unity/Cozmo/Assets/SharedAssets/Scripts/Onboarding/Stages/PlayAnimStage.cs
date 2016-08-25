using Cozmo.UI;
using UnityEngine;
using Anki.Cozmo;

namespace Onboarding {

  public class PlayAnimStage : OnboardingBaseStage {

    [SerializeField]
    private SerializableAnimationTrigger _Animation;

    private IRobot _CurrentRobot;
    public override void Start() {
      base.Start();
      _CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Onboarding);
      _CurrentRobot.SendAnimationTrigger(_Animation.Value, HandleEndAnimationComplete);
    }

    public override void OnDestroy() {
      base.OnDestroy();
      _CurrentRobot.ExecuteBehavior(BehaviorType.NoneBehavior);
    }

    public override void SkipPressed() {
      OnboardingManager.Instance.GoToNextStage();
    }

    private void HandleEndAnimationComplete(bool success) {
      // If we saw a face recently, look at it and play the "hello player"
      // Otherwise just play the "hello world" and be impressed with the enviornment.
      if (_CurrentRobot.Faces.Count > 0) {
        _CurrentRobot.TurnTowardsFacePose(_CurrentRobot.Faces[0], 4.3f, 10, HandleTurnedTowardsFace);
      }
      else {
        _CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingHelloWorld, HandleReactionEndAnimationComplete);
      }
    }

    private void HandleTurnedTowardsFace(bool success) {
      _CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingHelloPlayer, HandleReactionEndAnimationComplete);
    }

    private void HandleReactionEndAnimationComplete(bool success) {
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}

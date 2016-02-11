using UnityEngine;
using System.Collections;
using Anki.Cozmo.Audio;

namespace CodeBreaker {
  public class CodeBreakerEndState : State {

    LightCube[] _TargetCubes;
    string _CozmoAnimationName;
    string _ReadySlideTextLocKey;

    public CodeBreakerEndState(LightCube[] targetCubes, string cozmoAnimationName,
                               string readySlideTextLocKey) {
      _TargetCubes = targetCubes;
      _CozmoAnimationName = cozmoAnimationName;
      _ReadySlideTextLocKey = readySlideTextLocKey;
    }

    public override void Enter() {
      base.Enter();

      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
      _CurrentRobot.SendAnimation(_CozmoAnimationName, null);

      // Show slide
      CodeBreakerGame game = _StateMachine.GetGame() as CodeBreakerGame;
      game.ShowReadySlide(_ReadySlideTextLocKey,
        LocalizationKeys.kButtonAgain, HandleReadyButtonClicked);
    }

    private void HandleReadyButtonClicked() {
      _StateMachine.SetNextState(new PickCodeState(_TargetCubes));
    }
  }
}
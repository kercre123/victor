using UnityEngine;
using System.Collections;
using Anki.Cozmo.Audio;

namespace CodeBreaker {
  public class CodeBreakerEndState : State {

    LightCube[] _TargetCubes;
    Anki.Cozmo.AnimationTrigger _CozmoAnimationTrigger;
    string _ReadySlideTextLocKey;

    public CodeBreakerEndState(LightCube[] targetCubes, Anki.Cozmo.AnimationTrigger cozmoAnimationName,
                               string readySlideTextLocKey) {
      _TargetCubes = targetCubes;
      _CozmoAnimationTrigger = cozmoAnimationName;
      _ReadySlideTextLocKey = readySlideTextLocKey;
    }

    public override void Enter() {
      base.Enter();

      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.Cozmo_Connect);
      _CurrentRobot.SendAnimationTrigger(_CozmoAnimationTrigger);

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
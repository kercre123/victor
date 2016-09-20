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
      // TODO : This game is not MvP for ship and it looks like I would need to rewrite some stuff
      // to make this properly use the new Localization Key for playerwin 
      // (which now requires the player profile name as an arg). Leaving this TODO here to avoid
      // wasting time on this now and to help avoid wasting too much time for whoever passes through
      // this game in the future.
      _ReadySlideTextLocKey = readySlideTextLocKey;
    }

    public override void Enter() {
      base.Enter();

      GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect);
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
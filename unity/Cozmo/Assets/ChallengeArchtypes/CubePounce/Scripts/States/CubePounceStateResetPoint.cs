using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateResetPoint : CubePounceState {

    private CubePounceCheckPosition _CheckPosition = null;

    public override void Enter() {
      base.Enter();
      _CubePounceGame.ResetPounceChance();
      _CheckPosition = new CubePounceCheckPosition(_CurrentRobot, _CubePounceGame, CompleteAlignment);
      _CheckPosition.CheckAndFixPosition();
      _CubePounceGame.StopCycleCube(_CubePounceGame.GetCubeTarget().ID);
      _CubePounceGame.GetCubeTarget().SetLEDs(Color.green);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Tension);
    }

    private void CompleteAlignment() {
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetIn, HandleGetInAnimFinish);
    }

    private void HandleGetInAnimFinish(bool success) {
      _StateMachine.SetNextState(new CubePounceStateFakeOut());
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(HandleGetInAnimFinish);
      _CheckPosition.ClearCallbacks();
    }
  }
}

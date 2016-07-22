using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateResetPoint : CubePounceState {

    private bool _GetInAnimCompleted = false;
    private bool _CubeIsValid = false;

    public override void Enter() {
      base.Enter();
      _CubePounceGame.StopCycleCube(_CubePounceGame.GetCubeTarget().ID);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Tension);
      _CubePounceGame.ResetPounceChance();

      // If the cube hasn't been seen recently, call the cube missing functionality in cube pounce game
      if (_CubePounceGame.CubeSeenRecently) {
        ReactToCubeReturned();
      }
      else {
        ReactToCubeGone();
      }
    }

    private void ReactToCubeGone() {
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetOut, null);
      _CubePounceGame.GetCubeTarget().SetLEDs(Color.black);
      _CubeIsValid = false;
      _GetInAnimCompleted = false;
    }

    private void ReactToCubeReturned() {
      _CubePounceGame.GetCubeTarget().SetLEDs(Color.green);
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetIn, HandleGetInAnimFinish);
      _CubeIsValid = true;
    }

    public override void Update() {
      base.Update();

      if (_CubeIsValid && !_CubePounceGame.CubeSeenRecently) {
        ReactToCubeGone();
      }
      else if (!_CubeIsValid && _CubePounceGame.CubeSeenRecently) {
        ReactToCubeReturned();
      }

      if (_CubeIsValid && _GetInAnimCompleted && _CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistTight_mm)) {
        _StateMachine.SetNextState(_CubePounceGame.GetNextFakeoutOrAttemptState());
      }
    }

    private void HandleGetInAnimFinish(bool success) {
      _GetInAnimCompleted = true;
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(HandleGetInAnimFinish);
    }
  }
}

using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateResetPoint : CubePounceState {

    private bool _GetInAnimCompleted = false;
    private bool _GetInAnimInProgress = false;
    private bool _CubeIsValid = false;
    private bool _TurnInProgress = false;

    public override void Enter() {
      base.Enter();
      _CubePounceGame.StopCycleCube(_CubePounceGame.GetCubeTarget().ID);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Tension);
      _CubePounceGame.ResetPounceChance();

      // If the cube hasn't been seen recently, call the cube missing functionality in cube pounce game
      if (_CubePounceGame.CubeSeenRecently) {
        ReactToCubeReturned();

        if (!_CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistTight_mm)) {
          ReactToCubeOutOfRange();
        }
      }
      else {
        ReactToCubeGone();
        ReactToCubeOutOfRange();
      }
    }

    private void ReactToCubeGone() {
      _CubePounceGame.GetCubeTarget().SetLEDs(Color.black);
      _CubeIsValid = false;
    }

    private void ReactToCubeReturned() {
      _CubePounceGame.GetCubeTarget().SetLEDs(Color.green);
      _CubeIsValid = true;
    }

    private void ReactToCubeInRange() {
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetIn, HandleGetInAnimFinish);
      _GetInAnimInProgress = true;
    }

    private void ReactToCubeOutOfRange() {
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetOut, null);
      _GetInAnimCompleted = false;
    }

    private void TurnToCube() {
      Vector3 positionDifference = _CubePounceGame.GetCubeTarget().WorldPosition - _CurrentRobot.WorldPosition;
      float angleToCube_deg = Mathf.Rad2Deg * Mathf.Atan2(positionDifference.y, positionDifference.x);
      float angleDelta_deg = Mathf.DeltaAngle(_CurrentRobot.Rotation.eulerAngles.z, angleToCube_deg);
      _CurrentRobot.TurnInPlace(Mathf.Deg2Rad * angleDelta_deg, _CubePounceGame.GameConfig.TurnSpeed_rps, _CubePounceGame.GameConfig.TurnAcceleration_rps2, HandleTurnFinished);
      _TurnInProgress = true;
    }

    public override void Update() {
      base.Update();

      if (_CubeIsValid && !_CubePounceGame.CubeSeenRecently) {
        ReactToCubeGone();
        ReactToCubeOutOfRange();
      }
      else if (!_CubeIsValid && _CubePounceGame.CubeSeenRecently) {
        ReactToCubeReturned();
      }

      if (_CubeIsValid) {
        if (_TurnInProgress) {
          return;
        }

        if (!_CubePounceGame.WithinAngleTolerance()) {
          TurnToCube();
          return;
        }

        bool withinLooseCubePounceDistance = _CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistLoose_mm);
        if (_GetInAnimCompleted && !withinLooseCubePounceDistance) {
          ReactToCubeOutOfRange();
        }
        else if (!_GetInAnimCompleted && !_GetInAnimInProgress && _CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistTight_mm)) {
          ReactToCubeInRange();
        }

        if (_GetInAnimCompleted) {
          if (!_CubePounceGame.WithinAngleTolerance()) {
            TurnToCube();
            return;
          }

          if (withinLooseCubePounceDistance) {
            _StateMachine.SetNextState(_CubePounceGame.GetNextFakeoutOrAttemptState());
          }
        }
      }
    }

    private void HandleGetInAnimFinish(bool success) {
      _GetInAnimCompleted = true;
      _GetInAnimInProgress = false;
    }

    private void HandleTurnFinished(bool success) {
      _TurnInProgress = false;
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(HandleGetInAnimFinish);
      _CurrentRobot.CancelCallback(HandleTurnFinished);
    }
  }
}

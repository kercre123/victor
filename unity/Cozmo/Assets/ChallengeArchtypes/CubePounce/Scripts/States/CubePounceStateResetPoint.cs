using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateResetPoint : CubePounceState {

    private bool _GetReadyAnimCompleted = false;
    private bool _GetReadyAnimInProgress = false;
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

        if (!_CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistLoose_mm)) {
          ReactToCubeOutOfRange();
        }
      }
      else {
        ReactToCubeGone();
        ReactToCubeOutOfRange();
      }

      //TODO:(lc) this shouldn't be necessary, should be taken care of by animations
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue, null, Anki.Cozmo.QueueActionPosition.IN_PARALLEL);
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
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetReady, HandleGetInAnimFinish);

      // TODO:(lc) when these idles are ready (have head angle and lift height set correctly) enable them
      //_CurrentRobot.SetIdleAnimation(Anki.Cozmo.AnimationTrigger.CubePounceIdleLiftUp);

      _GetReadyAnimInProgress = true;
    }

    private void ReactToCubeOutOfRange() {

      // TODO:(lc) GetUnReady needs to end with lift down and head at correct angle, then can remove setting lift height manually
      //_CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetUnready, null);

      // TODO:(lc) when these idles are ready (have head angle and lift height set correctly) enable them
      //_CurrentRobot.SetIdleAnimation(Anki.Cozmo.AnimationTrigger.CubePounceIdleLiftDown);

      _CurrentRobot.SetLiftHeight(0.0f, null);

      _GetReadyAnimCompleted = false;
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

        if (!_GetReadyAnimCompleted && !_GetReadyAnimInProgress && _CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistLoose_mm)) {
          ReactToCubeInRange();
        }
        else if (_GetReadyAnimCompleted) {
          if (!_CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistLoose_mm)) {
            ReactToCubeOutOfRange();
          }
          else if (_CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistTight_mm)) {
            _StateMachine.SetNextState(new CubePounceStatePause());
          }
        }
      }
    }

    private void HandleGetInAnimFinish(bool success) {
      _GetReadyAnimCompleted = true;
      _GetReadyAnimInProgress = false;
    }

    private void HandleTurnFinished(bool success) {
      _TurnInProgress = false;
    }

    public override void Exit() {
      base.Exit();
      if (null != _CurrentRobot) {
        _CurrentRobot.CancelCallback(HandleGetInAnimFinish);
        _CurrentRobot.CancelCallback(HandleTurnFinished);
      }
    }
  }
}

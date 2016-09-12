using Anki.Cozmo.ExternalInterface;
using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateResetPoint : CubePounceState {

    private bool _GetReadyAnimCompleted = false;
    private bool _GetReadyAnimInProgress = false;
    private bool _CubeIsValid = false;
    private bool _TurnInProgress = false;
    private bool _BackupInProgress = false;
    private bool _GetUnreadyInProgress = false;
    private bool _IsPaused = false;
    private float _GetUnreadyStartAngle_deg;

    public override void Enter() {
      base.Enter();
      _CubePounceGame.StopCycleCube(_CubePounceGame.GetCubeTarget().ID);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Tension);

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

      _CurrentRobot.SetLiftHeight(0.0f, HandleGetUnreadyDone);

      _GetReadyAnimCompleted = false;
      _GetUnreadyInProgress = true;
      _GetUnreadyStartAngle_deg = _CurrentRobot.PitchAngle;
    }

    private void HandleGetUnreadyDone(bool success) {
      _GetUnreadyInProgress = false;

      if (!_IsPaused && _CubePounceGame.PitchIndicatesPounceSuccess(_GetUnreadyStartAngle_deg)) {
        DoBackupReaction();
      }
    }

    private void TurnToCube() {
      _CurrentRobot.TurnTowardsObject(_CubePounceGame.GetCubeTarget(), 
        headTrackWhenDone:      false, 
        maxPanSpeed_radPerSec:  _CubePounceGame.GameConfig.TurnSpeed_rps, 
        panAccel_radPerSec2:    _CubePounceGame.GameConfig.TurnAcceleration_rps2,
        callback:               HandleTurnFinished,
        setTiltTolerance_rad:   Mathf.PI
      );
      _TurnInProgress = true;
    }

    public override void Update() {
      base.Update();

      if (_BackupInProgress || _GetUnreadyInProgress || _IsPaused) {
        return;
      }

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
      _GetReadyAnimCompleted = success;
      _GetReadyAnimInProgress = false;
    }

    private void HandleTurnFinished(bool success) {
      _TurnInProgress = false;
    }

    private void DoBackupReaction() {
      // This is implemented as LiftUp + Wait + DriveStraight (backwards) instead of a simple unpowering
      // of the lift and drivestraight because of bug COZMO 3890, which causes DriveStraight actions when Cozmo
      // thinks he's picked up to fail
      RobotActionUnion[] actions = {
        new RobotActionUnion().Initialize(Singleton<SetLiftHeight>.Instance.Initialize(
          height_mm: CozmoUtil.kMaxLiftHeightMM,
          accel_rad_per_sec2: 5f,
          max_speed_rad_per_sec: 10f,
          duration_sec: 0f
        )),
        new RobotActionUnion().Initialize(Singleton<Wait>.Instance.Initialize(0.25f)),
        new RobotActionUnion().Initialize(Singleton<DriveStraight>.Instance.Initialize(500, -30, false))
      };
      _CurrentRobot.SendQueueCompoundAction(actions, HandleBackupComplete);
      _BackupInProgress = true;
    }

    private void HandleBackupComplete(bool success) {
      _BackupInProgress = false;
      _StateMachine.SetNextState(new CubePounceStatePostPoint(cozmoWon: true));
    }

    public override void Exit() {
      base.Exit();
      if (null != _CurrentRobot) {
        _CurrentRobot.CancelCallback(HandleGetInAnimFinish);
        _CurrentRobot.CancelCallback(HandleTurnFinished);
        _CurrentRobot.CancelCallback(HandleGetUnreadyDone);
        _CurrentRobot.CancelCallback(HandleBackupComplete);
      }
    }

    public override void Pause(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      _IsPaused = true;
    }

    public override void Resume(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      _IsPaused = false;
    }
  }
}

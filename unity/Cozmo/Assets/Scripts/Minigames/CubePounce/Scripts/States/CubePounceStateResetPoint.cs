using Anki.Cozmo.ExternalInterface;
using UnityEngine;
using System;
using Anki.Cozmo.Audio;

namespace Cozmo.Challenge.CubePounce {
  public class CubePounceStateResetPoint : CubePounceState {

    private bool _GetReadyAnimCompleted;
    private bool _GetReadyAnimInProgress = false;
    private bool _CubeIsValid = false;
    private bool _CubeInActiveRange = true;
    private bool _TurnInProgress = false;
    private bool _GetUnreadyInProgress = false;
    private float _CubeCreepTimerStart_s = -1f;
    private bool _LookForCubeInProgress = false;

    public const string kCubePounceResetPoint = "cube_pounce_satte_reset_point";

    public CubePounceStateResetPoint(bool overrideReadyAnimComplete = false) {
      _GetReadyAnimCompleted = overrideReadyAnimComplete;
    }

    public override void Enter() {
      base.Enter();
      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderPushCubeTowardCozmo);
      _CubePounceGame.SharedMinigameView.HideNarrowInfoTextSlide();

      // If the cube hasn't been seen recently, call the cube missing functionality in cube pounce game
      if (_CubePounceGame.CubeSeenRecently) {
        ReactToCubeReturned();

        if (!_CubePounceGame.WithinDistance(CubePounceGame.Zone.InPlay, CubePounceGame.DistanceType.Loose)) {
          ReactToCubeOutOfRange();
        }
      }
      else {
        ReactToCubeGone();
      }

      //TODO:(lc) this shouldn't be necessary, should be taken care of by animations
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue, null, Anki.Cozmo.QueueActionPosition.IN_PARALLEL);
    }

    private void ReactToCubeGone() {
      _CubePounceGame.GetCubeTarget().SetLEDs(Cozmo.UI.CubePalette.Instance.OutOfViewColor.lightColor);
      _CubeIsValid = false;

      if (_CubeInActiveRange) {
        float idealHeadAngle_rad = CozmoUtil.HeadAngleFactorToRadians(CozmoUtil.kIdealBlockViewHeadValue, useExactAngle: false);
        // COZMO-10908 We intentionally do not want to put the lift down here. The lift only goes down when Cozmo can see that the
        // cube is far away, NOT just because he can't see the cube at the moment. See ticket COZMO-9540 which mistakenly 
        // saw this behavior as a bug.
        _CurrentRobot.SearchForNearbyObject(_CubePounceGame.GetCubeTarget().ID, HandleLookForCube, headAngle_rad: idealHeadAngle_rad);
        _LookForCubeInProgress = true;
      }
      GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Minigame__Keep_Away_Between_Rounds);
    }

    private void HandleLookForCube(bool success) {
      _LookForCubeInProgress = false;
    }

    private void ReactToCubeReturned() {
      _CubeIsValid = true;

      if (_CubeInActiveRange) {
        _CubePounceGame.GetCubeTarget().SetLEDs(Cozmo.UI.CubePalette.Instance.ReadyColor.lightColor);
        GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Minigame__Keep_Away_Tension);
      }
      else {
        _CubePounceGame.GetCubeTarget().SetLEDs(Cozmo.UI.CubePalette.Instance.OutOfViewColor.lightColor);
        GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Minigame__Keep_Away_Between_Rounds);
      }
    }

    private void ReactToCubeInRange() {
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetReady, HandleGetInAnimFinish);
      _CurrentRobot.PushIdleAnimation(Anki.Cozmo.AnimationTrigger.CubePounceIdleLiftUp, kCubePounceResetPoint);

      _GetReadyAnimInProgress = true;

      // Determine what round it is
      int score = Math.Max(_CubePounceGame.CozmoScore, _CubePounceGame.HumanScore);
      score = Math.Min(_CubePounceGame.MaxScorePerRound - 1, score); // Last score is game point
      GameAudioClient.SetMusicRoundState(score + 1); // Offset for Audio Round State
      GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Minigame__Keep_Away_Tension);

      _CubeInActiveRange = true;
      _CubePounceGame.GetCubeTarget().SetLEDs(Cozmo.UI.CubePalette.Instance.ReadyColor.lightColor);
    }

    private void ReactToCubeOutOfRange() {
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetUnready, HandleGetUnreadyDone);
      _CurrentRobot.PushIdleAnimation(Anki.Cozmo.AnimationTrigger.CubePounceIdleLiftDown, kCubePounceResetPoint);

      _GetReadyAnimCompleted = false;
      _GetUnreadyInProgress = true;

      GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Minigame__Keep_Away_Between_Rounds);

      _CubeInActiveRange = false;
      _CubePounceGame.GetCubeTarget().SetLEDs(Cozmo.UI.CubePalette.Instance.OutOfViewColor.lightColor);
    }

    private void HandleGetUnreadyDone(bool success) {
      _GetUnreadyInProgress = false;
    }

    private void TurnToCube() {
      _CurrentRobot.TurnTowardsObject(_CubePounceGame.GetCubeTarget(),
        headTrackWhenDone: false,
        maxPanSpeed_radPerSec: _CubePounceGame.GameConfig.TurnSpeed_rps,
        panAccel_radPerSec2: _CubePounceGame.GameConfig.TurnAcceleration_rps2,
        callback: HandleTurnFinished,
        setTiltTolerance_rad: Mathf.PI
      );
      _TurnInProgress = true;
    }

    public override void Update() {
      base.Update();

      if (_GetUnreadyInProgress || _LookForCubeInProgress) {
        return;
      }

      if (_CubeIsValid && !_CubePounceGame.CubeSeenRecently) {
        ReactToCubeGone();
      }
      else if (!_CubeIsValid && _CubePounceGame.CubeSeenRecently) {
        ReactToCubeReturned();
      }

      if (_CubeIsValid) {
        if (_TurnInProgress) {
          return;
        }

        if (!_CubePounceGame.WithinAngleTolerance() && _CubeInActiveRange) {
          TurnToCube();
          return;
        }

        if (!_GetReadyAnimCompleted && !_GetReadyAnimInProgress && _CubePounceGame.WithinDistance(CubePounceGame.Zone.InPlay, CubePounceGame.DistanceType.Tight)) {
          ReactToCubeInRange();
        }
        else if (_GetReadyAnimCompleted) {
          if (!_CubePounceGame.WithinDistance(CubePounceGame.Zone.InPlay, CubePounceGame.DistanceType.Loose)) {
            ReactToCubeOutOfRange();
          }
          else if (_CubePounceGame.WithinDistance(CubePounceGame.Zone.Pounceable, CubePounceGame.DistanceType.Tight)) {
            _StateMachine.SetNextState(new CubePounceStatePause());
          }
          // If we aren't ready for cube creeping, clear the local timer
          else if (!_CubePounceGame.CubeReadyForCreep) {
            _CubeCreepTimerStart_s = -1f;
          }
          // Otherwise now we're ready for cube creeping, so start our local timer
          else {
            if (_CubeCreepTimerStart_s < 0.0f) {
              _CubeCreepTimerStart_s = Time.time + UnityEngine.Random.Range(_CubePounceGame.GameConfig.CreepDelayMinTime_s, _CubePounceGame.GameConfig.CreepDelayMaxTime_s);
            }

            if (Time.time >= _CubeCreepTimerStart_s) {
              float creepDistance = _CubePounceGame.GetNextCreepDistance();
              if (creepDistance > 0.0f) {
                _StateMachine.SetNextState(new CubePounceStateCreep(creepDistance));
              }
            }
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

    public override void Exit() {
      base.Exit();
      if (null != _CurrentRobot) {
        _CurrentRobot.CancelCallback(HandleGetInAnimFinish);
        _CurrentRobot.CancelCallback(HandleTurnFinished);
        _CurrentRobot.CancelCallback(HandleGetUnreadyDone);
        _CurrentRobot.CancelCallback(HandleLookForCube);
      }
    }
  }
}

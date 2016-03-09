using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapStatePlayNewHand : State {

    private const int _kMaxMisses = 2;

    private SpeedTapGame _SpeedTapGame = null;
    private float _StartTimeMs = -1.0f;
    private float _OnDelayTimeMs = 2000.0f;
    private float _OffDelayTimeMs = 2000.0f;
    private float _PeekMinTimeMs = 500.0f;
    private float _PeekMaxTimeMs = 1500.0f;
    private float _PeekDelayTimeMs = 0.0f;
    private float _MinCozmoTapDelayTimeMs = 380.0f;
    private float _MaxCozmoTapDelayTimeMs = 700.0f;
    private float _MatchProbability = 0.35f;
    private float _FakeProbability = 0.25f;
    private float _PeekProbability = 0.40f;

    private bool _LightsOn = false;
    private bool _GotMatch = false;
    private bool _CozmoTapping = false;
    private bool _TriedFake = false;
    private bool _TryFake = false;
    private bool _TryPeek = false;
    private bool _MidHand = false;
    private bool _PlayReady = false;
    private bool _CozmoTapRegistered = false;
    private float _CozmoTapDelayTimeMs = 0f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.CozmoBlock.SetLEDs(0, 0, 0xFF);
      _SpeedTapGame.PlayerBlock.SetLEDs(0, 0, 0xFF);
      _StartTimeMs = -1.0f;
      _LightsOn = false;
      _SpeedTapGame.PlayerTap = false;
      _MidHand = false;
      _PlayReady = false;
      _CozmoTapRegistered = false;

      _SpeedTapGame.CheckForAdjust(AdjustDone);
    }

    void AdjustDone(bool success) {
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(1.0f);
      _StartTimeMs = Time.time * 1000.0f;
      _PlayReady = true;
      if (_MidHand == false) {
        _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
        _SpeedTapGame.PlayerTappedBlockEvent += PlayerDidTap;
        _SpeedTapGame.CozmoTappedBlockEvent += CozmoDidTap;
        _MidHand = true;
      }
    }

    public override void Update() {
      base.Update();
      // Do not run game while not ready for play
      if (_PlayReady == false || _MidHand == false) {
        return;
      }
      float currTimeMs = Time.time * 1000.0f;

      if (_LightsOn) {
        if (_GotMatch) {
          if (!_CozmoTapping) {
            if ((currTimeMs - _StartTimeMs) >= _CozmoTapDelayTimeMs) { 
              _CurrentRobot.SendAnimationGroup(AnimationGroupName.kSpeedTap_Tap, RobotCompletedTapAnimation);
              _CozmoTapRegistered = false;
              _CozmoTapping = true;
              _MidHand = false;
            }
          }
        }
        else if (_TryFake) {
          if (!_TriedFake) {
            if ((currTimeMs - _StartTimeMs) >= _CozmoTapDelayTimeMs) { 
              _CurrentRobot.SendAnimationGroup(AnimationGroupName.kSpeedTap_Fake, AdjustCheck);
              _TriedFake = true;
              _TryPeek = false;
            }
          }
        }
        if ((currTimeMs - _StartTimeMs) >= _OnDelayTimeMs) {
          _SpeedTapGame.CozmoBlock.SetLEDs(0, 0, 0xFF);
          _SpeedTapGame.PlayerBlock.SetLEDs(0, 0, 0xFF);
          _LightsOn = false;
          _StartTimeMs = currTimeMs;
        }
      }
      else {
        if ((currTimeMs - _StartTimeMs) >= _OffDelayTimeMs) {
          RollForLights();
          _LightsOn = true;
          _StartTimeMs = currTimeMs;
        }
        else if (_TryPeek && (currTimeMs - _StartTimeMs) >= _PeekDelayTimeMs) {
          _TryPeek = false;
          _CurrentRobot.SendAnimationGroup(AnimationGroupName.kSpeedTap_Peek, AdjustCheck);
        }
      }
    }

    public override void Exit() {
      base.Exit();
      _SpeedTapGame.PlayerTappedBlockEvent -= PlayerDidTap;
      _SpeedTapGame.CozmoTappedBlockEvent -= CozmoDidTap;
    }

    void RobotCompletedTapAnimation(bool success) {
      DAS.Info("SpeedTapStatePlayNewHand.tap_complete", "");
      if (_CozmoTapRegistered) {
        _SpeedTapGame.ConsecutiveMisses = 0;
      }
      else {
        _SpeedTapGame.ConsecutiveMisses++;
        if (_SpeedTapGame.ConsecutiveMisses > _kMaxMisses) {
          //TODO: Enter Pause-reset-FindCube state.
          DAS.Warn("SpeedTapGame", "Too many consecutive misses!");
        }
      }
      // check for player tapped first here.
      if (_SpeedTapGame.PlayerTap == false) {
        _SpeedTapGame.CozmoWinsHand();
      }
    }

    void AdjustCheck(bool success) {
      _SpeedTapGame.CheckForAdjust();
    }

    void CozmoDidTap() {
      DAS.Info("SpeedTapStatePlayNewHand.cozmo_tap", "");
      if (_CozmoTapping) {
        _CozmoTapRegistered = true;
      }
    }

    void PlayerDidTap() {
      DAS.Info("SpeedTapStatePlayNewHand.player_tap", "");
      if (_GotMatch) {
        _SpeedTapGame.PlayerTap = true;
        _SpeedTapGame.PlayerWinsHand();
      }
      else if (_LightsOn) {
        _SpeedTapGame.CozmoWinsHand();
      }
    }

    private void RollForLights() {
      _SpeedTapGame.RollingBlocks();
      _TriedFake = false;
      _CozmoTapDelayTimeMs = UnityEngine.Random.Range(_MinCozmoTapDelayTimeMs, _MaxCozmoTapDelayTimeMs);
      _SpeedTapGame.PlayerTap = false;

      _TryFake = UnityEngine.Random.Range(0.0f, 1.0f) < _FakeProbability;
      _TryPeek = UnityEngine.Random.Range(0.0f, 1.0f) < _PeekProbability;
      if (_TryPeek) {
        _PeekDelayTimeMs = UnityEngine.Random.Range(_PeekMinTimeMs, _PeekMaxTimeMs);
      }
      float matchExperiment = UnityEngine.Random.value;
      _GotMatch = matchExperiment < _MatchProbability;
      _SpeedTapGame.Rules.SetLights(_GotMatch, _SpeedTapGame);
    }

  }

}

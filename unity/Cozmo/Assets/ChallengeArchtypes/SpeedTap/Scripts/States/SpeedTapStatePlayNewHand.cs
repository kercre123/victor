using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapStatePlayNewHand : State {

    private const int _kMaxMisses = 3;
    // TODO: Remove this once we have a way to get timestamp information directly from animations
    private const float _kTapAnimHitDelay = 500.0f;

    private SpeedTapGame _SpeedTapGame = null;

    #region Difficulty Config Values

    private float _OnDurationMs;
    private float _OffDurationMs;
    private float _PeekMinTimeMs;
    private float _PeekMaxTimeMs;
    private float _MinCozmoTapDelayTimeMs;
    private float _MaxCozmoTapDelayTimeMs;
    private float _MatchProbability;
    private float _FakeProbability;

    #endregion

    // TODO: Replace Peek Probability with interval idle logic
    private float _PeekProbability = 0.80f;
    //

    private bool _LightsOn = false;
    private bool _GotMatch = false;
    private bool _CozmoTapping = false;
    private bool _TriedFake = false;
    private bool _TryFake = false;
    private bool _TryPeek = false;
    private bool _PlayReady = false;
    private bool _PlayerTapRegistered = false;
    private bool _CozmoTapRegistered = false;
    private bool _CozmoHitCube = false;
    private float _CozmoTapDelayTimeMs = 0f;
    private float _PeekDelayTimeMs;
    private float _StartTimeMs = -1.0f;
    private float _TapStartTimeMs = -1.0f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.CozmoBlock.SetLEDs(0, 0, 0xFF);
      _SpeedTapGame.PlayerBlock.SetLEDs(0, 0, 0xFF);
      _StartTimeMs = -1.0f;
      _LightsOn = false;
      _SpeedTapGame.PlayerHitFirst = false;
      _SpeedTapGame.MidHand = false;
      _PlayReady = false;
      _CozmoTapRegistered = false;
      _PlayerTapRegistered = false;
      _MatchProbability = _SpeedTapGame.BaseMatchChance;
      _FakeProbability = _SpeedTapGame.CozmoFakeoutChance;
      _PeekMinTimeMs = _SpeedTapGame.MinIdleIntervalMs;
      _PeekMaxTimeMs = _SpeedTapGame.MaxIdleIntervalMs;
      _MinCozmoTapDelayTimeMs = _SpeedTapGame.MinTapDelayMs;
      _MaxCozmoTapDelayTimeMs = _SpeedTapGame.MaxTapDelayMs;
      SetRoundValues();
      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapTap, RobotCompletedTapAnimation);
      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapFakeout, AdjustCheck);
      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapIdle, AdjustCheck);

      _SpeedTapGame.CheckForAdjust(AdjustDone);
    }

    private void SetRoundValues() {
      int currRound = _SpeedTapGame.CurrentRound;
      SpeedTapRoundData currRoundData = _SpeedTapGame.CurrentDifficultySettings.SpeedTapRoundSettings[currRound];
      _OnDurationMs = currRoundData.TimeHandDisplayed;
      _OffDurationMs = currRoundData.TimeBetweenHands;
      if (currRoundData.MidRoundMusic != Anki.Cozmo.Audio.GameState.Music.Invalid) {
        GameAudioClient.SetMusicState(currRoundData.MidRoundMusic);
      }
      else {
        GameAudioClient.SetMusicState(_SpeedTapGame.GetDefaultMusicState());
      }
    }

    public override void Exit() {
      base.Exit();
      _SpeedTapGame.PlayerTappedBlockEvent -= PlayerDidTap;
      _SpeedTapGame.CozmoTappedBlockEvent -= CozmoDidTap;
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapTap, RobotCompletedTapAnimation);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapFakeout, AdjustCheck);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapIdle, AdjustCheck);
    }

    void AdjustDone(bool success) {
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(1.0f);
      _StartTimeMs = Time.time * 1000.0f;
      _PlayReady = true;
      if (_SpeedTapGame.MidHand == false) {
        _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
        _SpeedTapGame.PlayerTappedBlockEvent += PlayerDidTap;
        _SpeedTapGame.CozmoTappedBlockEvent += CozmoDidTap;
        _SpeedTapGame.MidHand = true;
      }
    }

    public override void Update() {
      base.Update();
      float currTimeMs = Time.time * 1000.0f;
      // Do not run game while not ready for play
      if (_PlayReady == false || _SpeedTapGame.MidHand == false) {
        // If Tapping, check timing
        if (_CozmoTapping && (currTimeMs - _TapStartTimeMs) >= _kTapAnimHitDelay) {
          if (_SpeedTapGame.PlayerHitFirst == false) {
            _CozmoHitCube = true;
          }
        }
        return;
      }

      if (_LightsOn) {
        if (_GotMatch) {
          if (!_CozmoTapping) {
            if ((currTimeMs - _StartTimeMs) >= _CozmoTapDelayTimeMs) { 
              GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapTap);
              _CozmoTapRegistered = false;
              _CozmoHitCube = false;
              _CozmoTapping = true;
              _SpeedTapGame.MidHand = false;
              _TapStartTimeMs = currTimeMs;
            }
          }
        }
        else if (_TryFake) {
          if (!_TriedFake) {
            if ((currTimeMs - _StartTimeMs) >= _CozmoTapDelayTimeMs) { 
              GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapFakeout);
              _TriedFake = true;
              _TryPeek = false;
            }
          }
        }
        if ((currTimeMs - _StartTimeMs) >= _OnDurationMs) {
          _SpeedTapGame.CozmoBlock.SetLEDs(0, 0, 0xFF);
          _SpeedTapGame.PlayerBlock.SetLEDs(0, 0, 0xFF);
          _LightsOn = false;
          _StartTimeMs = currTimeMs;
        }
      }
      else {
        if ((currTimeMs - _StartTimeMs) >= _OffDurationMs) {
          RollForLights();
          _LightsOn = true;
          _StartTimeMs = currTimeMs;
        }
        else if (_TryPeek && (currTimeMs - _StartTimeMs) >= _PeekDelayTimeMs) {
          _TryPeek = false;
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapIdle);
        }
      }
    }
    // TODO:
    void RobotCompletedTapAnimation(bool success) {
      DAS.Info("SpeedTapStatePlayNewHand.tap_complete", "");
      // Checks if we have Registered a Tap on Cozmo's cube since starting the animation
      if (_CozmoTapRegistered) {
        _SpeedTapGame.ConsecutiveMisses = 0;
      }
      else {
        _SpeedTapGame.ConsecutiveMisses++;
        if (_SpeedTapGame.ConsecutiveMisses > _kMaxMisses) {
          //TODO: Enter Pause-reset-FindCube state. Possibly call out player for "cheating"
          DAS.Warn("SpeedTapGame", "Too many consecutive misses! Does Cozmo have a cube in front of them?");
        }
      }
      // check for player tapped first here.
      if (_SpeedTapGame.PlayerHitFirst == true) {
        _SpeedTapGame.PlayerWinsHand();
      }
      else {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLose);
        _SpeedTapGame.CozmoWinsHand();
      }
    }

    void AdjustCheck(bool success) {
      _SpeedTapGame.CheckForAdjust();
    }

    // Fires when Cozmo's cube registers a tap, false positives are likely, so do not attempt to use this
    // for
    void CozmoDidTap() {
      DAS.Info("SpeedTapStatePlayNewHand.cozmo_tap", "");
      if (_CozmoTapping) {
        _CozmoTapRegistered = true;
      }
    }

    void PlayerDidTap() {
      DAS.Info("SpeedTapStatePlayNewHand.player_tap", "");
      if (_PlayerTapRegistered) {
        return;
      }
      if (_GotMatch) {
        if (_CozmoHitCube == false) {
          _SpeedTapGame.PlayerHitFirst = true;
          _PlayerTapRegistered = true;
        }
      }
      else if (_LightsOn) {
        _SpeedTapGame.PlayerHitWrong();
        _PlayerTapRegistered = true;
      }
    }

    private void RollForLights() {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLightup);
      _TriedFake = false;
      _CozmoTapDelayTimeMs = UnityEngine.Random.Range(_MinCozmoTapDelayTimeMs, _MaxCozmoTapDelayTimeMs);
      _SpeedTapGame.PlayerHitFirst = false;

      _TryFake = UnityEngine.Random.Range(0.0f, 1.0f) < _FakeProbability;
      _TryPeek = UnityEngine.Random.Range(0.0f, 1.0f) < _PeekProbability;
      float matchExperiment = UnityEngine.Random.value;
      _GotMatch = matchExperiment < _MatchProbability;
      if (!_GotMatch) {
        _MatchProbability += _SpeedTapGame.MatchChanceIncrease;
        if (_TryPeek) {
          _PeekDelayTimeMs = UnityEngine.Random.Range(_PeekMinTimeMs, _PeekMaxTimeMs);
        }
      }
      _SpeedTapGame.Rules.SetLights(_GotMatch, _SpeedTapGame);
    }

  }

}

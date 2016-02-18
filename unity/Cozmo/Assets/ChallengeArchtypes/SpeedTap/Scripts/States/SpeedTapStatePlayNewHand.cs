using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapStatePlayNewHand : State {

    private SpeedTapGame _SpeedTapGame = null;
    private float _StartTimeMs = 0;
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
    private float _CozmoTapDelayTimeMs = 0f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _StartTimeMs = Time.time * 1000.0f;
      _SpeedTapGame.CozmoBlock.SetLEDs(0, 0, 0xFF);
      _SpeedTapGame.PlayerBlock.SetLEDs(0, 0, 0xFF);
      _LightsOn = false;

      GameAudioClient.SetMusicState(_SpeedTapGame.GetMusicState());

      _CurrentRobot.SetLiftHeight(1.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      _SpeedTapGame.PlayerTappedBlockEvent += PlayerDidTap;
    }

    public override void Update() {
      base.Update();

      float currTimeMs = Time.time * 1000.0f;

      if (_LightsOn) {
        if (_GotMatch) {
          if (!_CozmoTapping) {
            if ((currTimeMs - _StartTimeMs) >= _CozmoTapDelayTimeMs) { 
              _CurrentRobot.SendAnimation(RandomTap(), RobotCompletedTapAnimation);
              _CozmoTapping = true;
            }
          }
        }
        else if (_TryFake) {
          if (!_TriedFake) {
            if ((currTimeMs - _StartTimeMs) >= _CozmoTapDelayTimeMs) { 
              _CurrentRobot.SendAnimation(AnimationName.kSpeedTapFake, RobotCompletedFakeTapAnimation);
              _TriedFake = true;
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
          _CurrentRobot.SendAnimation(AnimationName.kSpeedTap_Peek);
        }
      }
    }

    private string RandomTap() {
      string tapName = "";
      int roll = UnityEngine.Random.Range(0, 4);
      switch (roll) {
      case 0:
        tapName = AnimationName.kSpeedTap_Tap_01;
        break;
      case 1:
        tapName = AnimationName.kSpeedTap_Tap_02;
        break;
      default:
        tapName = AnimationName.kSpeedTap_Tap_03;
        break;
      }
      return tapName;
    }

    public override void Exit() {
      base.Exit();
      GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
      _SpeedTapGame.PlayerTappedBlockEvent -= PlayerDidTap;
    }

    void RobotCompletedTapAnimation(bool success) {
      DAS.Info("SpeedTapStatePlayNewHand.tap_complete", "");
      // check for player tapped first here.
      CozmoDidTap();
    }

    void RobotCompletedFakeTapAnimation(bool success) {
      _CozmoTapping = false;
      _CurrentRobot.SetLiftHeight(1.0f);
    }

    void CozmoDidTap() {
      DAS.Info("SpeedTapStatePlayNewHand.cozmo_tap", "");
      _StateMachine.SetNextState(new SpeedTapCozmoWins());
    }

    void BlockTapped(int blockID, int numTaps) {
      if (blockID == _SpeedTapGame.PlayerBlock.ID) {
        PlayerDidTap();
      }   
    }

    void PlayerDidTap() {
      DAS.Info("SpeedTapStatePlayNewHand.player_tap", "");
      if (_GotMatch) {
        _StateMachine.SetNextState(new SpeedTapPlayerWins());
      }
      else if (_LightsOn) {
        _StateMachine.SetNextState(new SpeedTapPlayerLoses());
      }
    }

    private void RollForLights() {
      _SpeedTapGame.RollingBlocks();
      _TriedFake = false;
      _CozmoTapDelayTimeMs = UnityEngine.Random.Range(_MinCozmoTapDelayTimeMs, _MaxCozmoTapDelayTimeMs);

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

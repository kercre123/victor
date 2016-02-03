using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapStatePlayNewHand : State {

    private SpeedTapGame _SpeedTapGame = null;
    private float _StartTimeMs = 0;
    private float _OnDelayTimeMs = 2000.0f;
    private float _OffDelayTimeMs = 2000.0f;
    private float _MinCozmoTapDelayTimeMs = 380.0f;
    private float _MaxCozmoTapDelayTimeMs = 700.0f;
    private float _MatchProbability = 0.35f;

    private bool _LightsOn = false;
    private bool _GotMatch = false;
    private bool _CozmoTapping = false;
    private bool _TriedFake = false;
    private bool _TryFake = false;
    private float _CozmoTapDelayTimeMs = 0f;

    public override void Enter() {
      base.Enter();
      GameAudioClient.SetMusicState(MUSIC.PLAYFUL);
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _StartTimeMs = Time.time * 1000.0f;
      _SpeedTapGame.CozmoBlock.SetLEDs(0, 0, 0xFF);
      _SpeedTapGame.PlayerBlock.SetLEDs(0, 0, 0xFF);
      _LightsOn = false;

      _CurrentRobot.SetLiftHeight(1.0f);
      _CurrentRobot.SetHeadAngle(-0.5f);

      _SpeedTapGame.PlayerTappedBlockEvent += PlayerDidTap;
      RobotEngineManager.Instance.RobotCompletedAnimation += RobotCompletedTapAnimation;
    }

    public override void Update() {
      base.Update();

      float currTimeMs = Time.time * 1000.0f;

      if (_LightsOn) {
        if (_GotMatch) {
          if (!_CozmoTapping) {
            if ((currTimeMs - _StartTimeMs) >= _CozmoTapDelayTimeMs) { 
              _CurrentRobot.SendAnimation(AnimationName.kTapCube);
              _CozmoTapping = true;
            }
          }
        }
        else if (_TryFake) {
          if (!_TriedFake) {
            if ((currTimeMs - _StartTimeMs) >= _CozmoTapDelayTimeMs) { 
              _CurrentRobot.SendAnimation(AnimationName.kSpeedTapFake);
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
      }
    }

    public override void Exit() {
      base.Exit();
      GameAudioClient.SetMusicState(MUSIC.SILENCE);
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GenericEvent.PLAY_SFX_UI_POSITIVE_02);
      RobotEngineManager.Instance.RobotCompletedAnimation -= RobotCompletedTapAnimation;
      _SpeedTapGame.PlayerTappedBlockEvent -= PlayerDidTap;
    }

    void RobotCompletedTapAnimation(bool success, string animName) {
      DAS.Info("SpeedTapStatePlayNewHand.RobotCompletedTapAnimation", animName + " success = " + success);
      switch (animName) {
      case AnimationName.kTapCube:
        DAS.Info("SpeedTapStatePlayNewHand.tap_complete", "");
        // check for player tapped first here.
        CozmoDidTap();
        break;
      case AnimationName.kSpeedTapFake:
        _CozmoTapping = false;
        _CurrentRobot.SetLiftHeight(1.0f);
        break;
      }

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

      _TryFake = UnityEngine.Random.Range(0.0f, 1.0f) < 0.25f;

      float matchExperiment = UnityEngine.Random.value;
      _GotMatch = matchExperiment < _MatchProbability;
      _SpeedTapGame.Rules.SetLights(_GotMatch, _SpeedTapGame);
    }

  }

}

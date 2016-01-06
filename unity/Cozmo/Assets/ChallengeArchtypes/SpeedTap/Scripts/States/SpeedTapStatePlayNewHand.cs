using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapStatePlayNewHand : State {

    private SpeedTapGame _SpeedTapGame = null;
    private float _StartTimeMs = 0;
    private float _OnDelayTimeMs = 2000.0f;
    private float _OffDelayTimeMs = 2000.0f;
    private float _CozmoTapDelayTimeMs = 380.0f;
    private float _MatchProbability = 0.35f;

    private bool _LightsOn = false;
    private bool _GotMatch = false;
    private bool _CozmoTapping = false;

    public override void Enter() {
      base.Enter();
      GameAudioClient.SetMusicState(MusicGroupStates.PLAYFUL);
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
        else if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.25f) {
          if (!_CozmoTapping) {
            _CurrentRobot.SendAnimation(AnimationName.kSpeedTapFake);
            _CozmoTapping = true;
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
      GameAudioClient.SetMusicState(MusicGroupStates.SILENCE);
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_POSITIVE_02);
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

      float matchExperiment = UnityEngine.Random.value;
      _GotMatch = matchExperiment < _MatchProbability;
      _SpeedTapGame.Rules.SetLights(_GotMatch, _SpeedTapGame);
    }

  }

}

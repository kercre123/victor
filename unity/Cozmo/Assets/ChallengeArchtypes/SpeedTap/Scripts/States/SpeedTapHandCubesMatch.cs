using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesMatch : State {

    private SpeedTapGame _SpeedTapGame;

    private float _CozmoMovementDelay_sec;
    private float _StartTimestamp_sec;
    private bool _IsCozmoMoving;
    private bool _AnyTapRegistered;

    // TODO Change logic when animation keyframe is implemented
    private const float _kCozmoAnimationTapTime_sec = 0.5f;
    private float _StartTapAnimationTimestamp_sec;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      _StartTimestamp_sec = Time.time;
      _CozmoMovementDelay_sec = 0.001f * UnityEngine.Random.Range(_SpeedTapGame.MinTapDelayMs, _SpeedTapGame.MaxTapDelayMs);
      _IsCozmoMoving = false;
      _AnyTapRegistered = false;
      _StartTapAnimationTimestamp_sec = float.MinValue;

      // Set lights on cubes
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLightup);
      _SpeedTapGame.Rules.SetLights(shouldMatch: true, game: _SpeedTapGame);

      // Listen for player taps
      _SpeedTapGame.PlayerTappedBlockEvent += HandlePlayerTap;
    }

    public override void Update() {
      base.Update();

      // Check to tap after some time
      float secondsElapsed = Time.time - _StartTimestamp_sec;
      if (!_IsCozmoMoving && secondsElapsed > _CozmoMovementDelay_sec) {
        _IsCozmoMoving = true;
        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapTap);
        _StartTapAnimationTimestamp_sec = Time.time;
      }
      else if (_IsCozmoMoving && (Time.time - _StartTapAnimationTimestamp_sec) > _kCozmoAnimationTapTime_sec) {
        // TODO Change logic when animation keyframe is implemented
        // Move to react state with cozmo mistapping
        if (!_AnyTapRegistered) {
          _AnyTapRegistered = true;
          _StateMachine.SetNextState(new SpeedTapHandReactToPoint(PointWinner.Cozmo, false));
        }
      }
    }

    public override void Exit() {
      base.Exit();
      _SpeedTapGame.PlayerTappedBlockEvent -= HandlePlayerTap;
    }

    private void HandlePlayerTap() {
      // Move to react state with player mistapping
      if (!_AnyTapRegistered) {
        _AnyTapRegistered = true;
        _StateMachine.SetNextState(new SpeedTapHandReactToPoint(PointWinner.Player, false));
      }
    }
  }
}
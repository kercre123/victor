using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesMatch : State {

    private SpeedTapGame _SpeedTapGame;

    private float _CozmoMovementDelay_sec;
    private float _StartTimestamp_sec;
    private bool _IsCozmoMoving;
    private bool _AnyTapRegistered;


    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      _StartTimestamp_sec = Time.time;
      _CozmoMovementDelay_sec = 0.001f * UnityEngine.Random.Range(_SpeedTapGame.MinTapDelayMs, _SpeedTapGame.MaxTapDelayMs);
      _IsCozmoMoving = false;
      _AnyTapRegistered = false;

      // Set lights on cubes
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLightup);
      _SpeedTapGame.Rules.SetLights(shouldMatch: true, game: _SpeedTapGame);

      // Listen for player taps
      _SpeedTapGame.PlayerTappedBlockEvent += HandlePlayerTap;

      RobotEngineManager.Instance.OnRobotAnimationEvent += HandleRobotAnimationEvent;
    }

    public override void Update() {
      base.Update();

      // Check to tap after some time
      float secondsElapsed = Time.time - _StartTimestamp_sec;
      if (!_IsCozmoMoving && secondsElapsed > _CozmoMovementDelay_sec) {
        _IsCozmoMoving = true;

        // All the taps should have a "TAPPED_BLOCK" RobotAnimationEvent.
        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapTap);
      }
    }

    public override void Exit() {
      base.Exit();
      _SpeedTapGame.PlayerTappedBlockEvent -= HandlePlayerTap;
      RobotEngineManager.Instance.OnRobotAnimationEvent += HandleRobotAnimationEvent;
    }

    private void HandleRobotAnimationEvent(Anki.Cozmo.ExternalInterface.AnimationEvent msg) {
      if (msg.event_id == Anki.Cozmo.AnimEvent.TAPPED_BLOCK && !_AnyTapRegistered) {
        _AnyTapRegistered = true;
        _StateMachine.SetNextState(new SpeedTapHandReactToPoint(PointWinner.Cozmo, false));
      }
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
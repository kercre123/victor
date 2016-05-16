using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesMatch : State {

    private SpeedTapGame _SpeedTapGame;
    private const float kResultsCheckDelay = 500.0f;

    private float _CozmoMovementDelay_sec;
    private float _StartTimestamp_sec;
    private float _EndTimestamp_sec;
    private bool _IsCozmoMoving;


    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.ResetTapTimestamps();

      _StartTimestamp_sec = Time.time;
      _EndTimestamp_sec = -1;
      _CozmoMovementDelay_sec = 0.001f * UnityEngine.Random.Range(_SpeedTapGame.MinTapDelayMs, _SpeedTapGame.MaxTapDelayMs);
      _IsCozmoMoving = false;

      // Set lights on cubes
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLightup);
      _SpeedTapGame.Rules.SetLights(shouldMatch: true, game: _SpeedTapGame);

      // Listen for player taps
      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapTap, HandleRobotAnimEnd);
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
      else if (_EndTimestamp_sec != -1 && Time.time - _EndTimestamp_sec > kResultsCheckDelay) {
        ResolveHand();
      }
    }

    public override void Exit() {
      base.Exit();
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapTap, HandleRobotAnimEnd);
    }

    private void HandleRobotAnimEnd(bool success) {
      _EndTimestamp_sec = Time.time;
    }

    /// <summary>
    /// Resolves the hand, first tapper wins, if no taps, we keep going,
    /// the no tap case should never happen but you never know with Cozmo.
    /// </summary>
    private void ResolveHand() {
      switch (_SpeedTapGame.FirstTapper) {
      case FirstToTap.Cozmo:
        _StateMachine.SetNextState(new SpeedTapHandReactToPoint(PointWinner.Cozmo, false));
        break;
      case FirstToTap.Player:
        _StateMachine.SetNextState(new SpeedTapHandReactToPoint(PointWinner.Player, false));
        break;
      case FirstToTap.NoTaps:
      default:
        _StateMachine.SetNextState(new SpeedTapHandCubesOff());
        break;
      }
    }
  }
}
using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesMatch : State {

    private SpeedTapGame _SpeedTapGame;
    private const float kResultsCheckDelay = 0.02f;

    private float _CozmoMovementDelay_sec;
    private float _StartTimestamp_sec;
    private float _EndTimestamp_sec;
    private float _LightsOnDuration;
    private bool _IsCozmoMoving;


    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.ResetTapTimestamps();
      _LightsOnDuration = _SpeedTapGame.GetLightsOnDurationSec();
      _StartTimestamp_sec = Time.time;
      _EndTimestamp_sec = -1;

      // Reaction time should be relative to the LightsOnDuration
      _CozmoMovementDelay_sec = 0.001f * _LightsOnDuration * UnityEngine.Random.Range(_SpeedTapGame.MinTapDelayMs, _SpeedTapGame.MaxTapDelayMs);
      // Cap delays to prevent issues, but fire a warning
      if (_CozmoMovementDelay_sec > _LightsOnDuration) {
        DAS.Warn("SpeedTapHandCubesMatch.Enter", "TapDelay is greater than _LightsOnDuration, Skill Configs are likely wrong");
        _CozmoMovementDelay_sec = _LightsOnDuration;
      }
      _IsCozmoMoving = false;

      // Set lights on cubes
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLightup);
      _SpeedTapGame.Rules.SetLights(shouldMatch: true, game: _SpeedTapGame);
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
      else if (_IsCozmoMoving && _SpeedTapGame.FirstTapper != FirstToTap.NoTaps && _EndTimestamp_sec == -1) {
        // If any taps have been registered and cozmo has attempted to move, immediately set the end timestamp
        // in order to make Resolve hand more responsive when receiving player taps significantly before Cozmo
        // taps. Still we use the kResultsCheckDelay in order to prevent issues with messages being received
        // in a different order than their actual timestamps.
        _EndTimestamp_sec = Time.time;
      }
      else if (_EndTimestamp_sec != -1 && Time.time - _EndTimestamp_sec > kResultsCheckDelay) {
        ResolveHand();
      }
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
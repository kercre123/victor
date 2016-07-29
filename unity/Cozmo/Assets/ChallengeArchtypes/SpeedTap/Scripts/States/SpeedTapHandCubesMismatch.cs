using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesMismatch : State {

    private SpeedTapGame _SpeedTapGame;

    private float _LightsOnDuration_sec;
    private float _CozmoMovementDelay_sec;
    private float _StartTimestamp_sec;
    private float _EndTimestamp_sec = -1;
    private bool _IsCozmoMoving;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.ResetTapTimestamps();

      _StartTimestamp_sec = Time.time;
      _LightsOnDuration_sec = _SpeedTapGame.GetLightsOnDurationSec();
      _CozmoMovementDelay_sec = (_LightsOnDuration_sec * UnityEngine.Random.Range(_SpeedTapGame.MinTapDelay_percent, _SpeedTapGame.MaxTapDelay_percent));
      _IsCozmoMoving = false;

      // Set lights on cubes
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.Gp_St_Lightup);
      _SpeedTapGame.Rules.SetLights(shouldMatch: false, game: _SpeedTapGame);
    }

    public override void Update() {
      base.Update();

      // Check to tap after some time
      float secondsElapsed = Time.time - _StartTimestamp_sec;
      if (!_IsCozmoMoving && secondsElapsed > _CozmoMovementDelay_sec) {
        _IsCozmoMoving = true;
        DoCozmoMovement();
      }
      else if (((_SpeedTapGame.FirstTapper != FirstToTap.NoTaps) || (secondsElapsed > _LightsOnDuration_sec))
               && _EndTimestamp_sec == -1) {
        // If any taps have been registered immediately set the end timestamp
        // in order to make Resolve hand more responsive when receiving player taps significantly before Cozmo
        // taps. Still we use the TapResolutionDelay in order to prevent issues with messages being received
        // in a different order than their actual timestamps.
        _EndTimestamp_sec = Time.time;
      }
      else if (_EndTimestamp_sec != -1 && Time.time - _EndTimestamp_sec > _SpeedTapGame.TapResolutionDelay) {
        ResolveHand();
      }
    }

    public override void Exit() {
      base.Exit();
    }

    private void HandleCozmoFakeoutAnimationEnd(bool success) {
      _SpeedTapGame.CheckForAdjust();
    }

    private void DoCozmoMovement() {
      // Mistake or fakeout or nothing?
      float randomPercent = UnityEngine.Random.Range(0f, 1f);
      if (randomPercent < _SpeedTapGame.CozmoMistakeChance) {
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSpeedtapTap);
      }
      else {
        randomPercent -= _SpeedTapGame.CozmoMistakeChance;
        if (randomPercent < _SpeedTapGame.CozmoFakeoutChance) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSpeedtapFakeout, HandleCozmoFakeoutAnimationEnd);
        }
      }
    }

    /// <summary>
    /// Resolves the hand, first tapper loses, if no taps, we keep going.
    /// </summary>
    private void ResolveHand() {
      switch (_SpeedTapGame.FirstTapper) {
      case FirstToTap.Cozmo:
        _StateMachine.SetNextState(new SpeedTapHandReactToPoint(PointWinner.Player, true));
        break;
      case FirstToTap.Player:
        _StateMachine.SetNextState(new SpeedTapHandReactToPoint(PointWinner.Cozmo, true));
        break;
      case FirstToTap.NoTaps:
      default:
        _StateMachine.SetNextState(new SpeedTapHandCubesOff());
        break;
      }
    }
  }
}
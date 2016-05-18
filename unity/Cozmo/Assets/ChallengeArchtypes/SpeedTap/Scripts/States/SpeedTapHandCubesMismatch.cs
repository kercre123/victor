using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesMismatch : State {
    
    private SpeedTapGame _SpeedTapGame;

    private float _LightsOnDuration_sec;
    private float _CozmoMovementDelay_sec;
    private float _StartTimestamp_sec;
    private bool _IsCozmoMoving;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.ResetTapTimestamps();

      _StartTimestamp_sec = Time.time;
      _LightsOnDuration_sec = _SpeedTapGame.GetLightsOnDurationSec();
      _CozmoMovementDelay_sec = (_LightsOnDuration_sec * UnityEngine.Random.Range(_SpeedTapGame.MinTapDelay, _SpeedTapGame.MaxTapDelay)) - _SpeedTapGame.CozmoTapLatency_sec;
      _IsCozmoMoving = false;

      // Set lights on cubes
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLightup);
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

      // Check to turn off cubes after some time
      if (secondsElapsed > _LightsOnDuration_sec) {
        // Move to turn off state
        ResolveHand();
      }
    }

    public override void Exit() {
      base.Exit();
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapFakeout, HandleCozmoFakeoutAnimationEnd);
    }

    private void HandleCozmoFakeoutAnimationEnd(bool success) {
      _SpeedTapGame.CheckForAdjust();
    }

    private void DoCozmoMovement() {
      // Mistake or fakeout or nothing?
      float randomPercent = UnityEngine.Random.Range(0f, 1f);
      if (randomPercent < _SpeedTapGame.CozmoMistakeChance) {
        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapTap);
      }
      else {
        randomPercent -= _SpeedTapGame.CozmoMistakeChance;
        if (randomPercent < _SpeedTapGame.CozmoFakeoutChance) {
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapFakeout, HandleCozmoFakeoutAnimationEnd);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapFakeout);
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
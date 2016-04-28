using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesMismatch : State {
    
    private SpeedTapGame _SpeedTapGame;

    private float _OnDuration_sec;
    private float _CozmoMovementDelay_sec;
    private float _StartTimestamp_sec;
    private bool _IsCozmoMoving;
    private bool _AnyTapRegistered;

    // TODO Change logic when animation keyframe is implemented
    private float _kCozmoAnimationTapTime_sec = 0.5f;
    private float _StartTapAnimationTimestamp_sec;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      _StartTimestamp_sec = Time.time;
      _OnDuration_sec = _SpeedTapGame.GetLightsOnDurationSec();
      _CozmoMovementDelay_sec = 0.001f * UnityEngine.Random.Range(_SpeedTapGame.MinTapDelayMs, _SpeedTapGame.MaxTapDelayMs);
      _IsCozmoMoving = false;
      _AnyTapRegistered = false;
      _StartTapAnimationTimestamp_sec = float.MinValue;

      // Set lights on cubes
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLightup);
      _SpeedTapGame.Rules.SetLights(shouldTap: false, game: _SpeedTapGame);

      // Listen for player taps
      _SpeedTapGame.PlayerTappedBlockEvent += HandlePlayerTap;
    }

    public override void Update() {
      base.Update();

      // Check to tap after some time
      float secondsElapsed = Time.time - _StartTimestamp_sec;
      if (!_IsCozmoMoving && secondsElapsed > _CozmoMovementDelay_sec) {
        _IsCozmoMoving = true;
        DoCozmoMovement();
      }
      else if (_IsCozmoMoving && _StartTapAnimationTimestamp_sec != float.MinValue
               && (Time.time - _StartTapAnimationTimestamp_sec) > _kCozmoAnimationTapTime_sec) {
        // TODO Change logic when animation keyframe is implemented
        // Move to react state with cozmo mistapping
        if (!_AnyTapRegistered) {
          _AnyTapRegistered = true;
          _StateMachine.SetNextState(new SpeedTapHandReactToPoint(PointWinner.PLAYER, true));
        }
      }

      // Check to turn off cubes after some time
      if (secondsElapsed > _OnDuration_sec) {
        // Move to turn off state
        _StateMachine.SetNextState(new SpeedTapHandCubesOff());
      }
    }

    public override void Exit() {
      base.Exit();
      _SpeedTapGame.PlayerTappedBlockEvent -= HandlePlayerTap;
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapFakeout, HandleCozmoFakeoutAnimationEnd);
    }

    private void HandleCozmoFakeoutAnimationEnd(bool success) {
      _SpeedTapGame.CheckForAdjust();
    }

    private void HandlePlayerTap() {
      // Move to react state with player mistapping
      if (!_AnyTapRegistered) {
        _AnyTapRegistered = true;
        _StateMachine.SetNextState(new SpeedTapHandReactToPoint(PointWinner.COZMO, true));
      }
    }

    private void DoCozmoMovement() {
      // Mistake or fakeout or nothing?
      float randomPercent = UnityEngine.Random.Range(0f, 1f);
      if (randomPercent < _SpeedTapGame.CozmoMistakeChance) {
        // TODO Change logic when animation keyframe is implemented
        // Favor the player if Cozmo makes a mistake
        _SpeedTapGame.PlayerTappedBlockEvent -= HandlePlayerTap;

        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapTap);
        _StartTapAnimationTimestamp_sec = Time.time;
      }
      else {
        randomPercent -= _SpeedTapGame.CozmoMistakeChance;
        if (randomPercent < _SpeedTapGame.CozmoFakeoutChance) {
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapFakeout, HandleCozmoFakeoutAnimationEnd);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapFakeout);
        }
      }
    }
  }
}
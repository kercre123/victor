using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesOff : State {

    private SpeedTapGame _SpeedTapGame;

    private float _CubeOffStartTimestamp_sec;
    private float _OffDuration_sec;

    private float _IdleDelayStartTimestamp_sec;
    private float _IdleDelay_sec;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      _SpeedTapGame.CozmoBlock.SetLEDs(0);
      _SpeedTapGame.PlayerBlock.SetLEDs(0);

      _CubeOffStartTimestamp_sec = Time.time;
      _OffDuration_sec = _SpeedTapGame.GetLightsOffDurationSec();

      _IdleDelayStartTimestamp_sec = float.MinValue;
      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapIdle, HandleIdleAnimationEnd);
      _SpeedTapGame.CheckForAdjust(HandleAdjustEnd);

      _SpeedTapGame.StartRoundMusic();
    }

    public override void Update() {
      base.Update();
      UpdateIdleAnimation();
      CheckExitCubeOffState();
    }

    public override void Exit() {
      base.Exit();

      // Cancel animation callbacks if any
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapIdle, HandleIdleAnimationEnd);
      _SpeedTapGame.CurrentRobot.CancelCallback(HandleIdleAnimationEnd);
      _SpeedTapGame.CurrentRobot.CancelCallback(HandleAdjustEnd);
    }

    private void StartIdleAnimationCycle() {
      // Figure out the next time we should peek
      _IdleDelay_sec = 0.001f * UnityEngine.Random.Range(_SpeedTapGame.MinIdleIntervalMs, _SpeedTapGame.MaxIdleIntervalMs);
        
      // Reset peek timer
      _IdleDelayStartTimestamp_sec = Time.time;
    }

    private void UpdateIdleAnimation() {
      // After a delay, trigger a peek animation
      if (_IdleDelayStartTimestamp_sec != float.MinValue
          && (Time.time - _IdleDelayStartTimestamp_sec) > _IdleDelay_sec) {
        // Don't send another peek animation
        _IdleDelayStartTimestamp_sec = float.MinValue;
        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapIdle);
      }
    }

    private void HandleIdleAnimationEnd(bool success) {
      _SpeedTapGame.CheckForAdjust(HandleAdjustEnd);
    }

    private void HandleAdjustEnd(bool success) {
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(1.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      StartIdleAnimationCycle();
    }

    private void CheckExitCubeOffState() {
      // Exit state after allotted off time
      if ((Time.time - _CubeOffStartTimestamp_sec) > _OffDuration_sec) {
        RollForMatch();
      }
    }

    private void RollForMatch() {
      bool doMatch = UnityEngine.Random.value < _SpeedTapGame.CurrentMatchChance;
      if (doMatch) {
        _SpeedTapGame.CurrentMatchChance = _SpeedTapGame.BaseMatchChance;
        _StateMachine.SetNextState(new SpeedTapHandCubesMatch());
      }
      else {
        _SpeedTapGame.CurrentMatchChance += _SpeedTapGame.MatchChanceIncrease;
        _StateMachine.SetNextState(new SpeedTapHandCubesMismatch());
      }
    }
  }
}
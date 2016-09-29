using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesOff : State {

    private SpeedTapGame _SpeedTapGame;

    private float _CubeOffStartTimestamp_sec;
    private float _OffDuration_sec;

    private float _PeekDelayStartTimestamp_sec;
    private float _PeekDelay_sec;

    private bool _IsPlayingPeekAnimation = false;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.RedMatch = false;

      _SpeedTapGame.SetLEDs(_SpeedTapGame.CozmoBlockID, Color.black);
      _SpeedTapGame.SetLEDs(_SpeedTapGame.PlayerBlockID, Color.black);

      _CubeOffStartTimestamp_sec = Time.time;
      _OffDuration_sec = _SpeedTapGame.GetLightsOffDurationSec();

      _PeekDelayStartTimestamp_sec = float.MinValue;
      _SpeedTapGame.CheckForAdjust(HandleAdjustEnd);

      _SpeedTapGame.StartRoundMusic();
    }

    public override void Update() {
      base.Update();
      UpdatePeekAnimation();
      CheckExitCubeOffState();
    }

    public override void Exit() {
      base.Exit();

      // Cancel animation callbacks if any
      if (_SpeedTapGame.CurrentRobot != null) {
        _SpeedTapGame.CurrentRobot.CancelCallback(HandlePeekAnimationEnd);
        _SpeedTapGame.CurrentRobot.CancelCallback(HandleAdjustEnd);
      }
    }

    private void StartPeekAnimationCycle() {
      // Figure out the next time we should peek
      _PeekDelay_sec = _OffDuration_sec * UnityEngine.Random.Range(_SpeedTapGame.MinIdleInterval_percent, _SpeedTapGame.MaxIdleInterval_percent);

      // Reset peek timer
      _PeekDelayStartTimestamp_sec = Time.time;
    }

    private void UpdatePeekAnimation() {
      // After a delay, trigger a peek animation
      if (_PeekDelayStartTimestamp_sec != float.MinValue
          && (Time.time - _PeekDelayStartTimestamp_sec) > _PeekDelay_sec) {
        // Don't send another peek animation
        _PeekDelayStartTimestamp_sec = float.MinValue;
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSpeedtapIdle, HandlePeekAnimationEnd);
        _IsPlayingPeekAnimation = true;
      }
    }

    private void HandlePeekAnimationEnd(bool success) {
      _SpeedTapGame.CheckForAdjust(HandleAdjustEnd);
    }

    private void HandleAdjustEnd(bool success) {
      _IsPlayingPeekAnimation = false;
      StartPeekAnimationCycle();
    }

    private void CheckExitCubeOffState() {
      // Exit state after allotted off time
      if ((Time.time - _CubeOffStartTimestamp_sec) > _OffDuration_sec && !_IsPlayingPeekAnimation) {
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
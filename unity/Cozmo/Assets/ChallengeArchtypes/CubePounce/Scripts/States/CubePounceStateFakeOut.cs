using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateFakeOut : CubePounceState {
    
    private float _FakeOutDelay_s;
    private float _FirstTimestamp = -1;
    private CubePounceCheckPosition _CheckPosition = null;
    public bool _FakeOutTriggered = false;

    public override void Enter() {
      base.Enter();
      _CheckPosition = new CubePounceCheckPosition(_CurrentRobot, _CubePounceGame, TransitionToNext);

      GameAudioClient.SetMusicState(_CubePounceGame.GetDefaultMusicState());
      _FakeOutDelay_s = _CubePounceGame.GetAttemptDelay();
      _FirstTimestamp = Time.time;

      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderWaitForPounce);
      _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoWaitForPounce);
    }

    public override void Update() {
      base.Update();
      if (!_FakeOutTriggered && (Time.time - _FirstTimestamp) > _FakeOutDelay_s) {
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceFake, HandleFakeoutEnd);
        _CubePounceGame.IncreasePounceChance();
        _FakeOutTriggered = true;
      }
    }

    private void HandleFakeoutEnd(bool success) {
      _CheckPosition.CheckAndFixPosition();
    }

    private void TransitionToNext() {
      if (_CubePounceGame.ShouldAttemptPounce()) {
        _StateMachine.SetNextState(new CubePounceStateAttempt());
      }
      else {
        _StateMachine.SetNextState(new CubePounceStateFakeOut());
      }
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(HandleFakeoutEnd);
      _CheckPosition.ClearCallbacks();
    }

    protected override void HandleOurCubeMoved(float accX, float accY, float aaZ) {
      _StateMachine.SetNextState(new CubePounceStatePostPoint(cozmoWon: true));
    }
  }
}

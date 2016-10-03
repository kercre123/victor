using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateFakeOut : CubePounceState {

    public override void Enter() {
      base.Enter();

      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderWaitForPounce);
      _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoWaitForPounce);

      _CubePounceGame.CurrentlyInFakeoutState = true;

      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceFake, HandleFakeoutEnd);
      _CubePounceGame.IncreasePounceChance();
    }

    private void HandleFakeoutEnd(bool success) {
      _StateMachine.SetNextState(new CubePounceStatePause());
    }

    public override void Exit() {
      base.Exit();
      if (_CurrentRobot != null) {
        _CurrentRobot.CancelCallback(HandleFakeoutEnd);
      }
      _CubePounceGame.CurrentlyInFakeoutState = false;
    }
  }
}

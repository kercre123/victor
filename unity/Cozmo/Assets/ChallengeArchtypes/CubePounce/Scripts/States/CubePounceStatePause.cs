using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStatePause : CubePounceState {
    
    private float _FakeOutDelay_s;
    private float _FirstTimestamp = -1;

    public override void Enter() {
      base.Enter();

      GameAudioClient.SetMusicState(_CubePounceGame.GetDefaultMusicState());

      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderWaitForPounce);
      _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoWaitForPounce);

      _FakeOutDelay_s = _CubePounceGame.GetAttemptDelay();
      _FirstTimestamp = Time.time;
    }

    public override void Update() {
      base.Update();

      if (!_CubePounceGame.CubeSeenRecently || !_CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistTight_mm)) {
        _StateMachine.SetNextState(new CubePounceStateResetPoint());
        return;
      }

      if (_CubePounceGame.WithinPounceDistance(_CubePounceGame.CubePlaceDistTooClose_mm)) {
        _StateMachine.SetNextState(new CubePounceStateAttempt(useClosePounce: true));
      }

      if ((Time.time - _FirstTimestamp) > _FakeOutDelay_s) {
        _StateMachine.SetNextState(_CubePounceGame.GetNextFakeoutOrAttemptState());
      }
    }
  }
}

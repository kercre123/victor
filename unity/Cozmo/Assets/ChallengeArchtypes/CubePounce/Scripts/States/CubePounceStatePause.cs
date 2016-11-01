using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStatePause : CubePounceState {

    private float _FakeOutDelay_s;
    private float _FirstTimestamp = -1;

    public override void Enter() {
      base.Enter();

      _FakeOutDelay_s = _CubePounceGame.GetAttemptDelay();
      _FirstTimestamp = Time.time;
    }

    public override void Update() {
      base.Update();

      if (!_CubePounceGame.CubeSeenRecently || !_CubePounceGame.WithinDistance(CubePounceGame.Zone.Pounceable, CubePounceGame.DistanceType.Loose)) {
        _StateMachine.SetNextState(new CubePounceStateResetPoint());
        return;
      }

      if (_CubePounceGame.WithinDistance(CubePounceGame.Zone.MousetrapClose, CubePounceGame.DistanceType.Exact)) {
        _StateMachine.SetNextState(new CubePounceStateAttempt(useClosePounce: true));
      }

      if ((Time.time - _FirstTimestamp) > _FakeOutDelay_s) {
        _StateMachine.SetNextState(_CubePounceGame.GetNextFakeoutOrAttemptState());
      }
    }
  }
}

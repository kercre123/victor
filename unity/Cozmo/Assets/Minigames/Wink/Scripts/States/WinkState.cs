using UnityEngine;
using System.Collections;

namespace Wink {
  public class WinkState : State {

    private WinkGame _WinkGame;
    private float _EnterWinkStateTime;

    public override void Enter() {
      base.Enter();
      _WinkGame = _StateMachine.GetGame() as WinkGame;
      _WinkGame.PickNewWinkSide();
      _EnterWinkStateTime = Time.time;
    }

    public override void Update() {
      base.Update();
      if (_WinkGame.GetWinkCompleted()) {
        _StateMachine.SetNextState(new CelebrateState());
      }
      else if (Time.time - _EnterWinkStateTime > 10.0f) {
        _StateMachine.SetNextState(new AngryState());
      }
    }

    public override void Exit() {
      base.Exit();
      _WinkGame.DoneWink();
    }
  }

}

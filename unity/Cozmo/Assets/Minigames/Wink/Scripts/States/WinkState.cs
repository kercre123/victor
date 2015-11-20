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
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kMajorWin, OnAnimationFinished);
        _StateMachine.SetNextState(animState);
      }
      else if (Time.time - _EnterWinkStateTime > 10.0f) {
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kShocked, OnAnimationFinished);
        _StateMachine.SetNextState(animState);
      }
    }

    private void OnAnimationFinished(bool success) {
      _StateMachine.SetNextState(new WinkState());
    }

    public override void Exit() {
      base.Exit();
      _WinkGame.DoneWink();
    }
  }

}

using UnityEngine;
using System.Collections;

namespace Wink {
  public class WinkState : State {

    private WinkGame _WinkGame;
    private float _EnterWinkStateTime;
    private bool _WinkSuccess = false;

    public override void Enter() {
      base.Enter();
      _WinkGame = _StateMachine.GetGame() as WinkGame;
      _WinkGame.PickNewWinkSide();
      _EnterWinkStateTime = Time.time;
    }

    public override void Update() {
      base.Update();
      if (_WinkGame.GetWinkCompleted()) {
        // Success if we trigger motion detection and get Cozmo's attention.
        _WinkSuccess = true;
        _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorWin, OnAnimationFinished));
      }
      else if (Time.time - _EnterWinkStateTime > _WinkGame.TimeLimit) {
        // Failure if motion is not detected within time frame.
        _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, OnAnimationFinished));
      }
    }

    private void OnAnimationFinished(bool success) {
      bool gameDone = false;
      if (_WinkSuccess) {
        gameDone = _WinkGame.WaveSuccess();
      }
      else if (!_WinkGame.TryDecrementAttempts()) {
        _WinkGame.RaiseMiniGameLose();
      }
      if (!gameDone) {
        _StateMachine.SetNextState(new WinkState());
      }
    }

    public override void Exit() {
      base.Exit();
      _WinkGame.DoneWink();
    }
  }

}

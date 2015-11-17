using UnityEngine;
using System.Collections;

namespace Vortex {

  /*
   * This state controls when Cozmo Taps ( currently completely random )
   * while the spinner is going in the spinpanel.
   */
  public class StateSpinWait : State {
    private float _WaitTime = 0;
    private VortexGame _VortexGame = null;

    public override void Enter() {
      base.Enter();
      _WaitTime = 1.0f;
      _VortexGame = _StateMachine.GetGame() as VortexGame;
    }

    public override void Update() {
      base.Update();
      if (_WaitTime > 0) {
        _WaitTime -= Time.deltaTime;
        if (_WaitTime < 0) {
          CozmoTap();
        }
      }
    }

    public override void Exit() {
      base.Exit();
    }

    private void CozmoTap() {
      // Can always add a bit more randomness here
      _CurrentRobot.SendAnimation(AnimationName.kTapCube);
      _VortexGame.HandleBlockTapped(0, Random.Range(1, 4));
    }
  }

}

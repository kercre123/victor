using UnityEngine;
using System.Collections;

namespace CubeSlap {
  // Wait for the cube to be in position, then wait for a tap before transferring into Lineup State
  public class SeekState : State {

    private CubeSlapGame _CubeSlapGame;

    public override void Enter() {
      base.Enter();
      _CubeSlapGame = (_StateMachine.GetGame() as CubeSlapGame);
      _CurrentRobot.SetHeadAngle(-0.75f);
      _CurrentRobot.SetLiftHeight(0.0f);
    }

    public override void Update() {
      base.Update();

      LightCube target = _CubeSlapGame.GetCurrentTarget();
      if (target != null) {
        target.SetLEDs(Color.blue);
        _StateMachine.SetNextState(new TapCubeState(new LineupState(), target.ID));
      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}


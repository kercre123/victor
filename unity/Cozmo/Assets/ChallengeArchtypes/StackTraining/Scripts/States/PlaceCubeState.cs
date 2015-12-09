using UnityEngine;
using System.Collections;

namespace StackTraining {
  public class PlaceCubeState : State {

    private Bounds _BottomCubeBounds = new Bounds(new Vector3(100f, 0f, CozmoUtil.kBlockLengthMM * 0.5f), new Vector3(100f, 20f, 20f));

    private StackTrainingGame _Game;

    public override void Enter() {
      base.Enter();
      _Game = _StateMachine.GetGame() as StackTrainingGame;

      _CurrentRobot.SetHeadAngle(0f);
      _CurrentRobot.SetLiftHeight(0f);
    }

    public override void Update() {
      base.Update();

      var bottomCube = _Game.BottomCube;
      var topCube = _Game.TopCube;

      if (bottomCube == null || topCube == null) {
        _Game.RaiseMiniGameLose();
        return;
      }

      Vector3 bottomCubePosition = _CurrentRobot.WorldToCozmo(bottomCube.WorldPosition);

      if (_BottomCubeBounds.Contains(bottomCubePosition) && bottomCube.MarkersVisible) {
        bottomCube.SetLEDs(Color.blue);

        _StateMachine.SetNextState(new TapCubeState(new HelpCozmoPickupState(), bottomCube.ID));
      }
      else {
        bottomCube.SetLEDs(Color.red);
        topCube.TurnLEDsOff();
      }
    }
  }

}

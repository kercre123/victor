using UnityEngine;
using System.Collections;

namespace Vortex {

  /*
   * This state comes up after we see the minimum amount of cupes ( 2 )
   * This state should do things like: 
   * show rules, 
   * pick player numbers ( right now just total seen cubes + cozmo ),
   * Wait for Cozmo to get to his block
  */
  public class StateIntro : State {

    private VortexGame _VortexGame = null;
    private bool _DrivingToBlock = false;
    private LightCube _CozmoBlock;

    public override void Enter() {
      base.Enter();

      _VortexGame = _StateMachine.GetGame() as VortexGame;
      // Stolen from SpeedTap so might want in a shared util?
      _CozmoBlock = GetClosestAvailableBlock();
    }

    public override void Update() {
      base.Update();

      DriveToBlock();
    }

    public override void Exit() {
      base.Exit();
    }

    private void DriveToBlock() {
      if (_DrivingToBlock)
        return;
      if (_CozmoBlock.MarkersVisible) {
        _DrivingToBlock = true;
        _CurrentRobot.AlignWithObject(_CozmoBlock, 90.0f, (bool success) => {
          _DrivingToBlock = false;
          if (success) {
            _VortexGame.OnIntroComplete(_CozmoBlock.ID);
          }
          else {
            DAS.Debug("SpeedTapStateGoToCube", "AlignWithObject Failed");
          }
        });
      }
    }

    private LightCube GetClosestAvailableBlock() {
      float minDist = float.MaxValue;
      ObservedObject closest = null;
      for (int i = 0; i < _CurrentRobot.SeenObjects.Count; ++i) {
        if (_CurrentRobot.SeenObjects[i] is LightCube) {
          float d = Vector3.Distance(_CurrentRobot.SeenObjects[i].WorldPosition, _CurrentRobot.WorldPosition);
          if (d < minDist) {
            minDist = d;
            closest = _CurrentRobot.SeenObjects[i];
          }
        }
      }
      return closest as LightCube;
    }

  }
}

using UnityEngine;
using Anki.Cozmo.ExternalInterface;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateCreep : CubePounceState {
    
    private float _CreepDistance_mm;

    public CubePounceStateCreep(float creepDistance_mm) {
      _CreepDistance_mm = creepDistance_mm;
    }

    public override void Enter() {
      base.Enter();

      _CurrentRobot.SendQueueSingleAction(Singleton<DriveStraight>.Instance.Initialize(500, _CreepDistance_mm, false), HandleDriveComplete);
    }

    private void HandleDriveComplete(bool success) {
      _StateMachine.SetNextState(new CubePounceStateResetPoint(overrideReadyAnimComplete:true));
    }

    public override void Exit() {
      base.Exit();
      if (null != _CurrentRobot) {
        _CurrentRobot.CancelCallback(HandleDriveComplete);
      }
    }
  }
}

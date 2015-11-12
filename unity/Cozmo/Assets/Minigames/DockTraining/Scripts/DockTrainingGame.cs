using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DockTraining {

  public class DockTrainingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    private LightCube _CurrentTarget = null;

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WaitForTargetState(), 2, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.StopFaceAwareness();

      CreateDefaultQuitButton();
    }

    // Update is called once per frame
    void Update() {
      _StateMachineManager.UpdateAllMachines();

      if (CurrentRobot.SeenObjects.Contains(_CurrentTarget) == false) {
        _CurrentTarget = FindNewTarget();
      }
    }

    void InitialCubesDone() {

    }

    public LightCube GetCurrentTarget() {
      return _CurrentTarget;
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }

    private LightCube FindNewTarget() {
      for (int i = 0; i < CurrentRobot.VisibleObjects.Count; ++i) {
        float distance = Vector3.Distance(CurrentRobot.WorldPosition, CurrentRobot.VisibleObjects[i].WorldPosition);
        float relDot = Vector3.Dot(CurrentRobot.Forward, (CurrentRobot.VisibleObjects[i].WorldPosition - CurrentRobot.WorldPosition).normalized);

        if (distance > 80.0f && relDot > 0.8f && CurrentRobot.VisibleObjects[i] is LightCube) {
          return CurrentRobot.VisibleObjects[i] as LightCube;
        }
      }
      return null;
    }
  }

}

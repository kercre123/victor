using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DockTraining {

  public class DockTrainingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    private LightCube _CurrentTarget = null;
    private float _LastSeenTargetTime = 0.0f;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WaitForTargetState(), 1, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      OpenMinigameView();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
      if (CurrentRobot.VisibleObjects.Contains(_CurrentTarget) == false) {
        if (Time.time - _LastSeenTargetTime > 2.0f) {
          if (_CurrentTarget != null) {
            _CurrentTarget.SetLEDs(0);
          }
          // we haven't seen the current target for more than 2 seconds
          // so let's try to find a new one.
          _CurrentTarget = FindNewTarget();
          _LastSeenTargetTime = Time.time;
        }
      }
      else {
        _LastSeenTargetTime = Time.time;
      }

      if (_CurrentTarget != null) {
        _CurrentTarget.SetLEDs(CozmoPalette.ColorToUInt(Color.white));
      }

    }

    private void InitialCubesDone() {

    }

    public LightCube GetCurrentTarget() {
      return _CurrentTarget;
    }

    protected override void CleanUpOnDestroy() {
    }

    public bool ShouldTryDock() {
      if (_CurrentTarget == null)
        return false;
      float distance = Vector2.Distance(CurrentRobot.WorldPosition, _CurrentTarget.WorldPosition);
      return (distance < 60.0f);
    }

    public bool ShouldTryDockSucceed() {
      if (_CurrentTarget == null)
        return false;
      // check to see if the robots forward vector is toward the cube enough to attempt a successful dock.
      float dotVal = Vector2.Dot(CurrentRobot.Forward, (_CurrentTarget.WorldPosition - CurrentRobot.WorldPosition).normalized);
      if (dotVal > 0.9f) {
        return true;
      }
      return false;
    }

    private LightCube FindNewTarget() {
      for (int i = 0; i < CurrentRobot.VisibleObjects.Count; ++i) {
        float distance = Vector3.Distance(CurrentRobot.WorldPosition, CurrentRobot.VisibleObjects[i].WorldPosition);
        float relDot = Vector3.Dot(CurrentRobot.Forward, (CurrentRobot.VisibleObjects[i].WorldPosition - CurrentRobot.WorldPosition).normalized);

        if (distance > 100.0f && relDot > 0.8f && CurrentRobot.VisibleObjects[i] is LightCube) {
          return CurrentRobot.VisibleObjects[i] as LightCube;
        }
      }
      return null;
    }
  }

}

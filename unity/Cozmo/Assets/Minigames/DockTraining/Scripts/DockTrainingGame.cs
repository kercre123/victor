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
      // TODO
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WaitForTargetState(), 2, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      CreateDefaultQuitButton();
    }

    // Update is called once per frame
    void Update() {
      _StateMachineManager.UpdateAllMachines();
      if (CurrentRobot.VisibleObjects.Contains(_CurrentTarget) == false) {
        if (Time.time - _LastSeenTargetTime > 1.0f) {
          if (_CurrentTarget != null) {
            _CurrentTarget.SetLEDs(0);
          }
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

        if (distance > 100.0f && relDot > 0.8f && CurrentRobot.VisibleObjects[i] is LightCube) {
          return CurrentRobot.VisibleObjects[i] as LightCube;
        }
      }
      return null;
    }
  }

}

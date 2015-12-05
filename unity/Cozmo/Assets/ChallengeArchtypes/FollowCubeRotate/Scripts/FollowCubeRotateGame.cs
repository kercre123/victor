using UnityEngine;
using System.Collections;

namespace FollowCubeRotate {
  public class FollowCubeRotateGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    private int _Lives = 3;

    public LightCube CurrentTarget { get; set; }

    public bool LeftReached { get; set; }

    public float LastTimeTargetSeen { get; private set; }

    public bool RightReached { get; set; }

    public float StartingAngle { get; set; }

    protected override void Initialize(MinigameConfigBase minigameConfig) {

      InitializeMinigameObjects();
    }

    protected void InitializeMinigameObjects() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeRotateGame", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WaitForNewCube(), 1, null);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

      LeftReached = false;
      RightReached = false;
      CurrentTarget = null;
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
      if (CurrentRobot.VisibleObjects.Contains(CurrentTarget)) {
        LastTimeTargetSeen = Time.time;
      }
    }

    public void LoseLife() {
      
      LeftReached = false;
      RightReached = false;

      if (_Lives > 0) {
        _Lives--;
      }
      else {
        RaiseMiniGameLose();
      }
    }

    public void FindNewCubeTarget() {
      foreach (ObservedObject visibleObj in CurrentRobot.VisibleObjects) {
        if (visibleObj is LightCube) {
          CurrentTarget = (LightCube)visibleObj;
          StartingAngle = CurrentRobot.PoseAngle;
          return;
        }
      }
    }

    protected override void CleanUpOnDestroy() {
    }
  }

}

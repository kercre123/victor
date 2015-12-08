using UnityEngine;
using System.Collections;

namespace FollowCube {

  public class FollowCubeGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    public int FailuresLeft { get; set; }

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      // TODO
      InitializeMinigameObjects();
      FailuresLeft = 10;
    }

    protected void InitializeMinigameObjects() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeState(), 1, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

    }

    protected override void CleanUpOnDestroy() {

    }

    // Update is called once per frame
    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    void InitialCubesDone() {
      SetSpeed();
    }

    void SetSpeed() {
      ForwardSpeed = 30.0f;
      DistanceMax = 130.0f;
      DistanceMin = 90.0f;
    }

  }

}

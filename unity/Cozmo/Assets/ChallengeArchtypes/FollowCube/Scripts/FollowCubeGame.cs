using UnityEngine;
using System.Collections;

namespace FollowCube {

  public class FollowCubeGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    public float NotSeenForgivenessThreshold = 2f;

    private int _FailuresLeft;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      // TODO
      InitializeMinigameObjects();
      _FailuresLeft = 10;
    }

    public enum FollowTask {
      Forwards,
      Backwards,
      TurnLeft,
      TurnRight,
      FollowDrive
    }

    public FollowTask CurrentFollowTask;

    protected void InitializeMinigameObjects() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeForwardState(), 1, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

    }

    protected override void CleanUpOnDestroy() {

    }

    public void FailedAttempt() {
      _FailuresLeft--;
      CurrentRobot.SendAnimation(AnimationName.kShocked, HandleFailedAnimation);
    }

    private void HandleFailedAnimation(bool success) {
      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(-1.0f);
      if (_FailuresLeft == 0) {
        (_StateMachine.GetGame() as FollowCubeGame).RaiseMiniGameLose();
        _StateMachineManager.RemoveStateMachine("FollowCubeStateMachine");
      }
      else {
        InitialCubesState initCubeState = new InitialCubesState();
        switch (CurrentFollowTask) {
        case FollowTask.Forwards:
          initCubeState.InitialCubeRequirements(new FollowCubeForwardState(), 1, true, InitialCubesDone);
          break;
        case FollowTask.Backwards:
          initCubeState.InitialCubeRequirements(new FollowCubeBackwardState(), 1, true, InitialCubesDone);
          break;
        case FollowTask.TurnLeft:
          initCubeState.InitialCubeRequirements(new FollowCubeTurnLeftState(), 1, true, InitialCubesDone);
          break;
        case FollowTask.TurnRight:
          initCubeState.InitialCubeRequirements(new FollowCubeTurnRightState(), 1, true, InitialCubesDone);
          break;
        case FollowTask.FollowDrive:
          initCubeState.InitialCubeRequirements(new FollowCubeForwardState(), 1, true, InitialCubesDone);
          break;
        }

        _StateMachine.SetNextState(initCubeState);
      }
    }

    // Update is called once per frame
    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    void InitialCubesDone() {
      SetSpeed();
      CurrentFollowTask = FollowTask.Forwards;
    }

    void SetSpeed() {
      ForwardSpeed = 30.0f;
      DistanceMax = 130.0f;
      DistanceMin = 90.0f;
    }

  }

}

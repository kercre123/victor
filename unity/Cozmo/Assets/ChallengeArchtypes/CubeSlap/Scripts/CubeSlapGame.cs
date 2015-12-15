using UnityEngine;
using System.Collections;

namespace CubeSlap {
  
  public class CubeSlapGame : GameBase {
    
    private float _MinSlapDelay;
    private float _MaxSlapDelay;
    private int _SuccessGoal;
    private int _SuccessCount;
    private bool _AttemptedSlap = false;
    private bool _CliffFlagTrown = false;

    private LightCube _CurrentTarget = null;
    private float _LastSeenTargetTime = -1;

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      CubeSlapConfig config = minigameConfig as CubeSlapConfig;
      MaxAttempts = config.MaxAttempts;
      _SuccessGoal = config.SuccessGoal;
      _MinSlapDelay = config.MinSlapDelay;
      _MaxSlapDelay = config.MaxSlapDelay;
      InitializeMinigameObjects();
    }

    protected void InitializeMinigameObjects() {

      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("CubeSlapStateMachine", _StateMachine);

      CurrentRobot.SetBehaviorSystem(false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      RobotEngineManager.Instance.OnCliffEvent += HandleCliffEvent;

      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new SeekState(), 1, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
    }

    private void InitialCubesDone() {
      _CurrentTarget = GetClosestAvailableBlock();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    protected override void CleanUpOnDestroy() {

      RobotEngineManager.Instance.OnCliffEvent -= HandleCliffEvent;
      
    }

    public LightCube GetCurrentTarget() {
      if (_CurrentTarget == null) {
        _CurrentTarget = GetClosestAvailableBlock();
      }
      return _CurrentTarget;
    }

    private LightCube GetClosestAvailableBlock() {
      float minDist = float.MaxValue;
      ObservedObject closest = null;

      for (int i = 0; i < CurrentRobot.SeenObjects.Count; ++i) {
        if (CurrentRobot.SeenObjects[i] is LightCube) {
          float d = Vector3.Distance(CurrentRobot.SeenObjects[i].WorldPosition, CurrentRobot.WorldPosition);
          if (d < minDist) {
            minDist = d;
            closest = CurrentRobot.SeenObjects[i];
          }
        }
      }
      return closest as LightCube;
    }

    // Attempt the pounce
    public void AttemptSlap() {
      // Enter Animation State to attempt a pounce.
      // Set Callback for HandleEndSlapAttempt
      _AttemptedSlap = true;
      _CliffFlagTrown = false;
      CurrentRobot.SendAnimation(AnimationName.kTapCube, HandleEndSlapAttempt); // TODO: Replace with new animation
    }

    private void HandleEndSlapAttempt(bool success) {
      _AttemptedSlap = false;
      // If the animation completes and the cube is beneath Cozmo,
      // Cozmo has won.
      if (_CliffFlagTrown) {
        OnFailure();
        return;
      }
      else {
        // If the animation completes Cozmo is not on top of the Cube,
        // The player has won this round
        OnSuccess();
      }
    }

    private void HandleCliffEvent() {
      if (_AttemptedSlap) {
        _CliffFlagTrown = true;
      }
      else {
        // Someone is tampering with Cozmo, DISQUALIFIED!!!
        OnFailure();
      }
    }

    public void OnSuccess() {
      _SuccessCount++;
      Progress = _SuccessCount / _SuccessGoal;
      if (_SuccessCount >= _SuccessGoal) {
        RaiseMiniGameWin();
      }
      else {
        _StateMachine.SetNextState(new SeekState());
      }
    }

    public void OnFailure() {
      if (TryDecrementAttempts()) {
        _StateMachine.SetNextState(new SeekState());
      }
      else {
        RaiseMiniGameLose();
      }
    }

    public float GetSlapDelay() {
      return Random.Range(_MinSlapDelay,_MaxSlapDelay);
    }

  }
}

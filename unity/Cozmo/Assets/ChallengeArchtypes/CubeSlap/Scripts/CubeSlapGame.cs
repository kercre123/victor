using UnityEngine;
using System.Collections;

namespace CubeSlap {
  
  public class CubeSlapGame : GameBase {
    
    private float _MinSlapDelay;
    private float _MaxSlapDelay;
    private int _SuccessGoal;
    private int _SuccessCount;

    private LightCube _CurrentTarget = null;

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

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);

      _StateMachine.SetNextState(new SeekState());

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    protected override void CleanUpOnDestroy() {
      
    }

    public LightCube GetCurrentTarget() {
      return _CurrentTarget;
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

    // Attempt the pounce
    public void AttemptSlap() {
    }

    public void OnSuccess() {
      _SuccessCount++;
      Progress = _SuccessCount / _SuccessGoal;
      if (_SuccessCount >= _SuccessGoal) {
        RaiseMiniGameWin();
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

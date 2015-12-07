using UnityEngine;
using System.Collections;

namespace Peekaboo {
  /// <summary>
  /// Game for building face tracking skills. Has config options to enable Cozmo wandering
  /// when not paying attention to a face.
  /// </summary>
  public class PeekGame : GameBase {
    
    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private int _PeekSuccessCount = 0;
    private int _PeekGoalTarget = 3;
    private float _MoveSpeed = 30.0f;

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    public bool WanderEnabled { get; set; }

    [SerializeField]
    private PeekGamePanel _GamePanelPrefab;
    private PeekGamePanel _GamePanel;

    protected override void Initialize(MinigameConfigBase minigameConfigData) {
      PeekGameConfig config = (minigameConfigData as PeekGameConfig);
      _PeekGoalTarget = config.goal;
      _MoveSpeed = config.moveSpeed;
      WanderEnabled = config.wanderEnabled;

      InitializeMinigameObjects();
    }

    public void PeekSuccess() {
      _PeekSuccessCount++;
      _GamePanel.SetPoints(_PeekSuccessCount);
      if (_PeekSuccessCount >= _PeekGoalTarget) {
        RaiseMiniGameWin();
      }
    }

    protected void InitializeMinigameObjects() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("PeekGameStateMachine", _StateMachine);
      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<PeekGamePanel>();
      _GamePanel.SetPoints(_PeekSuccessCount);

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
      if (WanderEnabled) {
        CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
      }
      else {
        CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      }
      _StateMachine.SetNextState(new LookingForFaceState());

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, false);
      SetSpeed();
    }

    void SetSpeed() {
      ForwardSpeed = _MoveSpeed;
      DistanceMax = 200.0f;
      DistanceMin = 100.0f;
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }


    protected override void CleanUpOnDestroy() {
      
    }
  }
}

using UnityEngine;
using System.Collections;

namespace Peekaboo {
  public class PeekGame : GameBase {
    
    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private int _PeekSuccessCount = 0;
    private int _PeekGoalTarget = 3;

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    [SerializeField]
    private PeekGamePanel _GamePanelPrefab;
    private PeekGamePanel _GamePanel;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfigData) {
      PeekGameConfig config = (minigameConfigData as PeekGameConfig);
      _PeekGoalTarget = config.goal;
    }

    public void PeekSuccess() {
      _PeekSuccessCount++;
      _GamePanel.SetPoints(_PeekSuccessCount);
      if (_PeekSuccessCount >= _PeekGoalTarget) {
        RaiseMiniGameWin();
      }
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("PeekGameStateMachine", _StateMachine);
      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<PeekGamePanel>();
      _GamePanel.SetPoints(_PeekSuccessCount);

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
      _StateMachine.SetNextState(new LookingForFaceState());

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, false);
      SetSpeed();
      CreateDefaultQuitButton();
    }

    void SetSpeed() {
      ForwardSpeed = 20.0f;
      DistanceMax = 140.0f;
      DistanceMin = 80.0f;
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }


    protected override void CleanUpOnDestroy() {
      
    }
  }
}

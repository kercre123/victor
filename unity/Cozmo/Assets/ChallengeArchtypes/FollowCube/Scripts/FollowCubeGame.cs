using UnityEngine;
using System.Collections;

namespace FollowCube {

  public class FollowCubeGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    [SerializeField]
    private FollowCubeGamePanel _GamePanelPrefab;

    private FollowCubeGamePanel _GamePanel;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      // TODO
      InitializeMinigameObjects();
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

      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<FollowCubeGamePanel>();
    }

    protected override void CleanUpOnDestroy() {
      if (_GamePanel != null) {
        UIManager.CloseViewImmediately(_GamePanel);
      }
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

    public void SetAttemptsLeft(int attemptsLeft) {
      _GamePanel.SetAttemptsLeft(attemptsLeft);
    }
  }

}

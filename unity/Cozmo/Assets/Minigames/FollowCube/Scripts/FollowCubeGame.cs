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

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {
      // TODO
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeState(), 1, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      _GamePanel = UIManager.OpenDialog(_GamePanelPrefab).GetComponent<FollowCubeGamePanel>();

      CreateDefaultQuitButton();
    }

    public override void CleanUp() {
      if (_GamePanel != null) {
        UIManager.CloseDialogImmediately(_GamePanel);
      }

      DestroyDefaultQuitButton();
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

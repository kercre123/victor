using UnityEngine;
using System.Collections;
using System;

public class SpeedTapGame : GameBase {

  public LightCube CozmoBlock;
  public LightCube PlayerBlock;
  public int CozmoScore;
  public int PlayerScore;

  public event Action PlayerTappedBlockEvent;

  private StateMachineManager stateMachineManager_ = new StateMachineManager();
  private StateMachine stateMachine_ = new StateMachine();

  [SerializeField]
  private SpeedTapPanel _GamePanelPrefab;
  private SpeedTapPanel _GamePanel;

  [SerializeField]
  private AudioClip _RollSound;

  // Use this for initialization
  void Start() { 
    DAS.Info("SpeedTapGame", "Game Created");

    stateMachine_.SetGameRef(this);
    stateMachineManager_.AddStateMachine("FollowCubeStateMachine", stateMachine_);
    InitialCubesState initCubeState = new InitialCubesState();
    initCubeState.InitialCubeRequirements(new SpeedTapStateGoToCube(), 2, InitialCubesDone);
    stateMachine_.SetNextState(initCubeState);

    robot.VisionWhileMoving(true);
    LightCube.TappedAction += BlockTapped;
    robot.StopFaceAwareness();
    robot.SetBehaviorSystem(false);
    _GamePanel = UIManager.OpenDialog(_GamePanelPrefab).GetComponent<SpeedTapPanel>();
    _GamePanel.TapButtonPressed += UIButtonTapped;
    UpdateUI();

    CreateDefaultQuitButton();
  }

  void Update() {
    stateMachineManager_.UpdateAllMachines();
  }

  public override void CleanUp() {
    if (_GamePanel != null) {
      UIManager.CloseDialogImmediately(_GamePanel);
    }
    DestroyDefaultQuitButton();
  }

  void InitialCubesDone() {
    CozmoBlock = GetClosestAvailableBlock();
    PlayerBlock = GetFarthestAvailableBlock();
  }

  public void UpdateUI() {
    _GamePanel.SetScoreText(CozmoScore, PlayerScore);
  }

  public void RollingBlocks() {
    AudioManager.PlayAudioClip(_RollSound);
  }

  private void UIButtonTapped() {
    if (PlayerTappedBlockEvent != null) {
      PlayerTappedBlockEvent();
    }
  }

  private void BlockTapped(int blockID, int tappedTimes) {
    DAS.Info("SpeedTapGame", "Player Block Tapped.");
    if (PlayerBlock != null && PlayerBlock.ID == blockID) {
      if (PlayerTappedBlockEvent != null) {
        PlayerTappedBlockEvent();
      }
    }
  }

  private LightCube GetClosestAvailableBlock() {
    float minDist = float.MaxValue;
    ObservedObject closest = null;

    for (int i = 0; i < robot.SeenObjects.Count; ++i) {
      if (robot.SeenObjects[i] is LightCube) {
        float d = Vector3.Distance(robot.SeenObjects[i].WorldPosition, robot.WorldPosition);
        if (d < minDist) {
          minDist = d;
          closest = robot.SeenObjects[i];
        }
      }
    }
    return closest as LightCube;
  }

  private LightCube GetFarthestAvailableBlock() {
    float maxDist = 0;
    ObservedObject farthest = null;

    for (int i = 0; i < robot.SeenObjects.Count; ++i) {
      if (robot.SeenObjects[i] is LightCube) {
        float d = Vector3.Distance(robot.SeenObjects[i].WorldPosition, robot.WorldPosition);
        if (d >= maxDist) {
          maxDist = d;
          farthest = robot.SeenObjects[i];
        }
      }
    }
    return farthest as LightCube;
  }
}

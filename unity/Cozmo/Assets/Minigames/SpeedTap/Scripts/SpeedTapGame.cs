using UnityEngine;
using System.Collections;
using System;

public class SpeedTapGame : GameBase {

  public LightCube cozmoBlock_;
  public LightCube playerBlock_;
  public int cozmoScore_;
  public int playerScore_;

  public event Action PlayerTappedBlockEvent;

  private StateMachineManager stateMachineManager_ = new StateMachineManager();
  private StateMachine stateMachine_ = new StateMachine();

  [SerializeField]
  private SpeedTapPanel gamePanelPrefab_;
  private SpeedTapPanel gamePanel_;

  [SerializeField]
  private AudioClip rollSound_;

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
    gamePanel_ = UIManager.OpenDialog(gamePanelPrefab_).GetComponent<SpeedTapPanel>();
    UpdateUI();

    CreateDefaultQuitButton();
  }

  void Update() {
    stateMachineManager_.UpdateAllMachines();
  }

  public override void CleanUp() {
    if (gamePanel_ != null) {
      UIManager.CloseDialogImmediately(gamePanel_);
    }
    DestroyDefaultQuitButton();
  }

  void InitialCubesDone() {
    cozmoBlock_ = GetClosestAvailableBlock();
    playerBlock_ = GetFarthestAvailableBlock();
  }

  public void UpdateUI() {
    gamePanel_.SetScoreText(cozmoScore_, playerScore_);
  }

  public void RollingBlocks() {
    AudioManager.PlayAudioClip(rollSound_);
  }

  private void BlockTapped(int blockID, int tappedTimes) {
    DAS.Info("SpeedTapGame", "Player Block Tapped.");
    if (playerBlock_ != null && playerBlock_.ID == blockID) {
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

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
    initCubeState.InitialCubeRequirements(new SpeedTapStateBeginMatch(), 2, InitialCubesDone);

    robot.VisionWhileMoving(true);
    LightCube.TappedAction += BlockTapped;
    robot.StopFaceAwareness();
    robot.SetBehaviorSystem(false);
    gamePanel_ = UIManager.OpenDialog(gamePanelPrefab_).GetComponent<SpeedTapPanel>();
    UpdateUI();
  }

  void InitialCubesDone() {
    
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

}

using UnityEngine;
using System.Collections;
using System;

public class SpeedTapGame : GameBase {

  public LightCube cozmoBlock_;
  public LightCube playerBlock_;
  public int cozmoScore_;
  public int playerScore_;

  public event Action PlayerTappedBlockEvent;

  [SerializeField]
  private SpeedTapPanel gamePanelPrefab_;
  private SpeedTapPanel gamePanel_;


  [SerializeField]
  private AudioClip rollSound_;

  // Use this for initialization
  void Start() { 
    DAS.Info("SpeedTapGame", "Game Created");

    robot.VisionWhileMoving(true);
    LightCube.TappedAction += BlockTapped;
    robot.StopFaceAwareness();
    robot.SetBehaviorSystem(false);
    gamePanel_ = UIManager.OpenDialog(gamePanelPrefab_).GetComponent<SpeedTapPanel>();
    UpdateUI();
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

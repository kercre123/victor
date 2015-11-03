using UnityEngine;
using System.Collections;
using System;

public class SpeedTapGame : GameBase {

  public ActiveBlock cozmoBlock_;
  public ActiveBlock playerBlock_;
  public int cozmoScore_;
  public int playerScore_;

  public event Action PlayerTappedBlockEvent;


  [SerializeField]
  private UnityEngine.UI.Text cozmoScoreField;

  [SerializeField]
  private UnityEngine.UI.Text playerScoreField;

  [SerializeField]
  private AudioClip rollSound_;

  // Use this for initialization
  void Start() { 
    DAS.Info("SpeedTapGame", "Game Created");

    robot.VisionWhileMoving(true);
    ActiveBlock.TappedAction += BlockTapped;
    robot.StopFaceAwareness();
    robot.SetBehaviorSystem(false);

    UpdateUI();
  }

  public void UpdateUI() {
    if (cozmoScoreField != null) {
      cozmoScoreField.text = cozmoScore_.ToString();
    }
    if (playerScoreField != null) {
      playerScoreField.text = playerScore_.ToString();
    }
  }

  public void RollingBlocks() {
    AudioManager.PlayAudioClip(rollSound_);
  }


  private void BlockTapped(int blockID, int tappedTimes) {
    Debug.Log("Ima tapped:" + blockID);
    if (playerBlock_ != null && playerBlock_.ID == blockID) {
      if (PlayerTappedBlockEvent != null) {
        PlayerTappedBlockEvent();
      }
    }
  }

}

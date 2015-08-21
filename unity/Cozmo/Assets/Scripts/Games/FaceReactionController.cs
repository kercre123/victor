using UnityEngine;
using System.Collections;

public class FaceReactionController : GameController {

  private bool gameOver = false;
  private bool gameReady = true;

  private FaceReactionState playState = FaceReactionState.SPIN_RANDOM;
  private FaceReactionState nextState = FaceReactionState.SPIN_RANDOM;
  private float playStateTimer = 0.0f;

  public enum FaceReactionState {
    SPIN_RANDOM,
    SEARCH_FOR_FACE,
    MOVE_TOWARD_FACE,
    REACT_TO_FACE
  }

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
  }

  protected override void OnDisable() {
    base.OnDisable();
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();
  }

  protected override void Exit_BUILDING() {
    base.Exit_BUILDING();
  }

  protected override void Enter_PRE_GAME() {
    base.Enter_PRE_GAME();
  }

  protected override void Update_PRE_GAME() {
    base.Update_PRE_GAME();
  }

  protected override void Exit_PRE_GAME() {
    base.Exit_PRE_GAME();
  }

  protected override void Enter_PLAYING() {
    base.Enter_PLAYING();
  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();
  }

  protected override void Exit_PLAYING(bool overrideStars = false) {
    base.Exit_PLAYING();
  }

  protected override void Enter_RESULTS() {
    base.Enter_RESULTS();
  }

  protected override void Update_RESULTS() {
    base.Update_RESULTS();
  }

  protected override void Exit_RESULTS() {
    base.Exit_RESULTS();
  }

  protected override bool IsPreGameCompleted() {
    return true;
  }

  protected override bool IsGameReady() {
    return gameReady;
  }

  protected override bool IsGameOver() {
    return gameOver;
  }

  protected override void RefreshHUD() {
    base.RefreshHUD();
  }


  private void EnterPlayState() {
    playStateTimer = 0.0f;
    switch (playState) {
    case FaceReactionState.SPIN_RANDOM:
      Enter_SPIN_RANDOM();
      break;
    case FaceReactionState.SEARCH_FOR_FACE:
      Enter_SEARCH_FOR_FACE();
      break;
    case FaceReactionState.MOVE_TOWARD_FACE:
      Enter_MOVE_TOWARD_FACE();
      break;
    case FaceReactionState.REACT_TO_FACE:
      Enter_REACT_TO_FACE();
      break;
    }
  }

  private void UpdatePlayState() {
    playStateTimer += Time.deltaTime;
    switch (playState) {
    case FaceReactionState.SPIN_RANDOM:
      Update_SPIN_RANDOM();
      break;
    case FaceReactionState.SEARCH_FOR_FACE:
      Update_SEARCH_FOR_FACE();
      break;
    case FaceReactionState.MOVE_TOWARD_FACE:
      Update_MOVE_TOWARD_FACE();
      break;
    case FaceReactionState.REACT_TO_FACE:
      Update_REACT_TO_FACE();
      break;
    }
  }

  private void ExitPlayState() {
    switch (playState) {
    case FaceReactionState.SPIN_RANDOM:
      Exit_SPIN_RANDOM();
      break;
    case FaceReactionState.SEARCH_FOR_FACE:
      Exit_SEARCH_FOR_FACE();
      break;
    case FaceReactionState.MOVE_TOWARD_FACE:
      Exit_MOVE_TOWARD_FACE();
      break;
    case FaceReactionState.REACT_TO_FACE:
      Exit_REACT_TO_FACE();
      break;
    }
  }

  private void Enter_SPIN_RANDOM() {
    
  }

  private void Update_SPIN_RANDOM() {
    
  }

  private void Exit_SPIN_RANDOM() {
    
  }

  private void Enter_SEARCH_FOR_FACE() {
    
  }

  private void Update_SEARCH_FOR_FACE() {
    
  }

  private void Exit_SEARCH_FOR_FACE() {
    
  }

  private void Enter_MOVE_TOWARD_FACE() {
    
  }

  private void Update_MOVE_TOWARD_FACE() {
    
  }

  private void Exit_MOVE_TOWARD_FACE() {
    
  }

  private void Enter_REACT_TO_FACE() {
    
  }

  private void Update_REACT_TO_FACE() {
    
  }

  private void Exit_REACT_TO_FACE() {
    
  }

}

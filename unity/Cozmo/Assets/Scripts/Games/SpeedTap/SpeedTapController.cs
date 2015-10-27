using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

public class SpeedTapController : GameController {

  private bool gameOver = false;
  private bool gameReady = false;

  public Text cozmoScoreField;
  public Text playerScoreField;

  public ActiveBlock cozmoBlock = null;
  public ActiveBlock playerBlock = null;
  public int playerScore = 0;
  public int cozmoScore = 0;

  private StateMachineManager stateMachineManager = new StateMachineManager();
  private StateMachine speedTapStateMachine = new StateMachine();

  public event Action PlayerTappedBlockEvent;

  public void PlayerTappedButton() {
    Debug.Log("Ima button tapped");
    if (PlayerTappedBlockEvent != null) {
      PlayerTappedBlockEvent();
    }
  }

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    ActiveBlock.TappedAction += BlockTapped;
    ActiveBlock.StoppedAction += BlockStopped;
    robot.StopFaceAwareness();
    robot.SetBehaviorSystem(false);

    foreach (KeyValuePair<int, ActiveBlock> curBlock in robot.activeBlocks) {
      curBlock.Value.SetLEDs(0, 0, 0xFF);
    }
  }

  void Start() {
    UpdateUI();
  }

  private void BlockTapped(int blockID, int tappedTimes) {
    Debug.Log("Ima tapped:" + blockID);
    if (playerBlock != null && playerBlock.ID == blockID) {
      if (PlayerTappedBlockEvent != null) {
        PlayerTappedBlockEvent();
      }
    }
  }

  private void BlockStopped(int blockID) {
    Debug.Log("Ima stopped:" + blockID);
  }

  public void UpdateUI() {
    if (cozmoScoreField != null) {
      cozmoScoreField.text = cozmoScore.ToString();
    }
    if (playerScoreField != null) {
      playerScoreField.text = playerScore.ToString();
    }
  }

  private ActiveBlock GetClosestAvailableBlock() {
    float minDist = float.MaxValue;
    ObservedObject closest = null;

    for (int i = 0; i < robot.seenObjects.Count; ++i) {
      if (robot.seenObjects[i] is ActiveBlock) {
        float d = Vector3.Distance(robot.seenObjects[i].WorldPosition, robot.WorldPosition);
        if (d < minDist) {
          minDist = d;
          closest = robot.seenObjects[i];
        }
      }
    }
    return closest as ActiveBlock;
  }

  private ActiveBlock GetFarthestAvailableBlock() {
    float maxDist = 0;
    ObservedObject farthest = null;

    for (int i = 0; i < robot.seenObjects.Count; ++i) {
      if (robot.seenObjects[i] is ActiveBlock) {
        float d = Vector3.Distance(robot.seenObjects[i].WorldPosition, robot.WorldPosition);
        if (d >= maxDist) {
          maxDist = d;
          farthest = robot.seenObjects[i];
        }
      }
    }
    return farthest as ActiveBlock;
  }

  protected override void OnDisable() {
    base.OnDisable();
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        if (activeBlock.Value.MarkersVisible) {
          activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.blue);
        }
        else {
          activeBlock.Value.lights[j].onColor = 0;
        }
      }
    }

    if (robot.activeBlocks.Count >= 2) {
      gameReady = true;
    }
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
    cozmoBlock = GetClosestAvailableBlock();
    Debug.Assert(cozmoBlock != null);
    playerBlock = GetFarthestAvailableBlock();
    Debug.Assert(playerBlock != null);

    speedTapStateMachine.SetGameController(this);
    speedTapStateMachine.SetNextState(new SpeedTapStateGoToCube());
    stateMachineManager.AddStateMachine("SpeedTapStateMachine", speedTapStateMachine);

    // set idle parameters
    Anki.Cozmo.LiveIdleAnimationParameter[] paramNames = {
      Anki.Cozmo.LiveIdleAnimationParameter.BodyMovementDurationMax_ms,
      Anki.Cozmo.LiveIdleAnimationParameter.BodyMovementStraightFraction,
      Anki.Cozmo.LiveIdleAnimationParameter.HeadAngleVariability_deg,
      Anki.Cozmo.LiveIdleAnimationParameter.LiftHeightVariability_mm
    };
    float[] paramValues = {
      0.0f,
      0.0f,
      5.0f,
      0.2f
    };

    CozmoEmotionManager.instance.SetLiveIdleAnimationParameters(paramNames, paramValues);
    // Now in 'approaching block' state

  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();

    // Core Game Loop...

    // inputs

    // update state machine
    stateMachineManager.UpdateAllMachines();

    // check end condition?

    // update UI

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

}

using UnityEngine;
using System.Collections;
using System;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapGame : GameBase {

    public LightCube CozmoBlock;
    public LightCube PlayerBlock;
    public int CozmoScore;
    public int PlayerScore;

    public event Action PlayerTappedBlockEvent;

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    [SerializeField]
    private SpeedTapPanel _GamePanelPrefab;
    private SpeedTapPanel _GamePanel;

    [SerializeField]
    private AudioClip _RollSound;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      InitializeMinigameObjects();
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() { 
      DAS.Info(this, "Game Created");

      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new SpeedTapWaitForCubePlace(), 2, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.VisionWhileMoving(true);
      LightCube.TappedAction += BlockTapped;
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetBehaviorSystem(false);
      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<SpeedTapPanel>();
      _GamePanel.TapButtonPressed += UIButtonTapped;
      UpdateUI();

      // set idle parameters
      /*Anki.Cozmo.LiveIdleAnimationParameter[] paramNames = {
        Anki.Cozmo.LiveIdleAnimationParameter.BodyMovementSpacingMin_ms,
        Anki.Cozmo.LiveIdleAnimationParameter.LiftMovementSpacingMin_ms,
        Anki.Cozmo.LiveIdleAnimationParameter.HeadAngleVariability_deg,
      };

      float[] paramValues = {
        float.MaxValue,
        float.MaxValue,
        50.0f
      };
      CurrentRobot.SetIdleAnimation("_LIVE_");
      CurrentRobot.SetLiveIdleAnimationParameters(paramNames, paramValues);*/
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    protected override void CleanUpOnDestroy() {
      if (_GamePanel != null) {
        UIManager.CloseViewImmediately(_GamePanel);
      }

      LightCube.TappedAction -= BlockTapped;
    }

    void InitialCubesDone() {
      CozmoBlock = GetClosestAvailableBlock();
      PlayerBlock = GetFarthestAvailableBlock();
    }

    public void UpdateUI() {
      _GamePanel.SetScoreText(CozmoScore, PlayerScore);
    }

    public void RollingBlocks() {
      AudioClient.Instance.PostEvent(Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_CLICK_GENERAL, Anki.Cozmo.Audio.GameObjectType.Default);
    }

    private void UIButtonTapped() {
      if (PlayerTappedBlockEvent != null) {
        PlayerTappedBlockEvent();
      }
    }

    private void BlockTapped(int blockID, int tappedTimes) {
      DAS.Info(this, "Player Block Tapped.");
      if (PlayerBlock != null && PlayerBlock.ID == blockID) {
        if (PlayerTappedBlockEvent != null) {
          PlayerTappedBlockEvent();
        }
      }
    }

    private LightCube GetClosestAvailableBlock() {
      float minDist = float.MaxValue;
      ObservedObject closest = null;

      for (int i = 0; i < CurrentRobot.SeenObjects.Count; ++i) {
        if (CurrentRobot.SeenObjects[i] is LightCube) {
          float d = Vector3.Distance(CurrentRobot.SeenObjects[i].WorldPosition, CurrentRobot.WorldPosition);
          if (d < minDist) {
            minDist = d;
            closest = CurrentRobot.SeenObjects[i];
          }
        }
      }
      return closest as LightCube;
    }

    private LightCube GetFarthestAvailableBlock() {
      float maxDist = 0;
      ObservedObject farthest = null;

      for (int i = 0; i < CurrentRobot.SeenObjects.Count; ++i) {
        if (CurrentRobot.SeenObjects[i] is LightCube) {
          float d = Vector3.Distance(CurrentRobot.SeenObjects[i].WorldPosition, CurrentRobot.WorldPosition);
          if (d >= maxDist) {
            maxDist = d;
            farthest = CurrentRobot.SeenObjects[i];
          }
        }
      }
      return farthest as LightCube;
    }
  }
}

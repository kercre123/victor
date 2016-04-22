using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;
using Cozmo.Util;

namespace Cozmo.HomeHub {
  public class HomeHub : HubWorldBase {

    public Transform[] RewardIcons = null;

    [SerializeField]
    private StartView _StartViewPrefab;
    private StartView _StartViewInstance;

    [SerializeField]
    private HomeView _HomeViewPrefab;
    private HomeView _HomeViewInstance;

    [SerializeField]
    private ChallengeDetailsDialog _ChallengeDetailsPrefab;
    private ChallengeDetailsDialog _ChallengeDetailsDialogInstance;

    [SerializeField]
    private ChallengeDataList _ChallengeDataList;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStatesById;

    private GameBase _MiniGameInstance;

    public GameBase MiniGameInstance { 
      get {
        return _MiniGameInstance;
      }
    }

    private CompletedChallengeData _CurrentChallengePlaying;

    private void RefreshChallengeUnlockInfo(Anki.Cozmo.UnlockId id, bool unlocked) {
      LoadChallengeData(_ChallengeDataList, out _ChallengeStatesById);
      if (_HomeViewInstance != null) {
        _HomeViewInstance.SetChallengeStates(_ChallengeStatesById);
      }
    }

    public override bool LoadHubWorld() {
      RobotEngineManager.Instance.OnRequestSetUnlockResult += RefreshChallengeUnlockInfo;
      LoadChallengeData(_ChallengeDataList, out _ChallengeStatesById);
      ShowStartView();
      return true;
    }

    public override bool DestroyHubWorld() {
      RobotEngineManager.Instance.OnRequestSetUnlockResult -= RefreshChallengeUnlockInfo;
      CloseMiniGameImmediately();

      // Deregister events
      // Destroy dialog if it exists
      if (_HomeViewInstance != null) {
        DeregisterDialogEvents();
        _HomeViewInstance.CloseViewImmediately();
      }

      if (_StartViewInstance != null) {
        _StartViewInstance.CloseViewImmediately();
      }
      return true;
    }

    private void ShowStartView() {
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
      _StartViewInstance = UIManager.OpenView(_StartViewPrefab);
      _StartViewInstance.OnConnectClicked += HandleConnectClicked;
    }

    private void HandleConnectClicked() {
      _StartViewInstance.CloseView();
      ShowTimelineDialog();
    }

    private void ShowTimelineDialog() {
      // Create dialog with the game prefabs
      _HomeViewInstance = UIManager.OpenView(_HomeViewPrefab, verticalCanvas: true);
      _HomeViewInstance.OnLockedChallengeClicked += HandleLockedChallengeClicked;
      _HomeViewInstance.OnUnlockedChallengeClicked += HandleUnlockedChallengeClicked;
      _HomeViewInstance.OnCompletedChallengeClicked += HandleCompletedChallengeClicked;
      _HomeViewInstance.OnEndSessionClicked += HandleSessionEndClicked;

      // Show the current state of challenges being locked/unlocked
      _HomeViewInstance.Initialize(_ChallengeStatesById, this);

      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(true);
      DailyGoalManager.Instance.MinigameConfirmed += HandleStartChallengeRequest;
    }

    private void HandleSessionEndClicked() {
      RobotEngineManager.Instance.CurrentRobot.ResetRobotState(() => {
        CloseTimelineDialog();
        ObservedObject charger = RobotEngineManager.Instance.CurrentRobot.GetCharger();
        if (charger != null) {
          RobotEngineManager.Instance.CurrentRobot.MountCharger(charger, HandleChargerMounted);
        }
        else {
          ShowStartView();
        }

      });
    }

    private void HandleChargerMounted(bool success) {
      ShowStartView();
    }

    private void HandleLockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      // Do nothing for now
    }

    private void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      OpenChallengeDetailsDialog(challengeClicked, buttonTransform);
    }

    private void HandleStartChallengeClicked(string challengeClicked) {
      _ChallengeDetailsDialogInstance.ChallengeStarted -= HandleStartChallengeClicked;

      // Keep track of the current challenge
      _CurrentChallengePlaying = new CompletedChallengeData() {
        ChallengeId = challengeClicked,
        StartTime = System.DateTime.UtcNow,
      };

      // Close dialog
      CloseTimelineDialog();
      _ChallengeDetailsDialogInstance.CloseViewImmediately();

      // Play minigame immediately
      PlayMinigame(_ChallengeStatesById[challengeClicked].Data);
    }

    private void HandleStartChallengeRequest(string challengeRequested) {

      // Keep track of the current challenge
      _CurrentChallengePlaying = new CompletedChallengeData() {
        ChallengeId = challengeRequested,
        StartTime = System.DateTime.UtcNow,
      };

      // Close dialog
      CloseTimelineDialog();

      // Play minigame immediately
      PlayMinigame(_ChallengeStatesById[challengeRequested].Data);
    }

    private void HandleCompletedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      OpenChallengeDetailsDialog(challengeClicked, buttonTransform);
    }

    private void OpenChallengeDetailsDialog(string challenge, Transform buttonTransform) {
      // We need to initialize the dialog first before opening the view, so don't animate right away
      _ChallengeDetailsDialogInstance = UIManager.OpenView(_ChallengeDetailsPrefab, 
        newView => {
          newView.Initialize(_ChallengeStatesById[challenge].Data, buttonTransform);
        }, 
        verticalCanvas: true);

      // React to when we should start the challenge.
      _ChallengeDetailsDialogInstance.ChallengeStarted += HandleStartChallengeClicked;
    }

    private void HandleMiniGameLose(StatContainer rewards, Transform[] rewardIcons) {
      HandleMiniGameCompleted(rewards, rewardIcons, didWin: false);
    }

    private void HandleMiniGameWin(StatContainer rewards, Transform[] rewardIcons) {
      HandleMiniGameCompleted(rewards, rewardIcons, didWin: true);
    }

    private void HandleMiniGameCompleted(StatContainer rewards, Transform[] rewardIcons, bool didWin) {
      // If we are in a challenge that needs to be completed, complete it
      if (_CurrentChallengePlaying != null) {
        CompleteChallenge(_CurrentChallengePlaying, didWin, rewards);
        _CurrentChallengePlaying = null;
      }
      ShowTimelineDialog();
      RewardIcons = rewardIcons;
    }

    private void HandleMiniGameQuit() {
      // Reset the current challenge
      _CurrentChallengePlaying = null;
      ShowTimelineDialog();
    }

    private void PlayMinigame(ChallengeData challengeData) {
      // Reset the robot behavior
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
      }

      GameObject newMiniGameObject = GameObject.Instantiate(challengeData.MinigamePrefab);
      _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
      _MiniGameInstance.InitializeMinigame(challengeData);
      _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;
      _MiniGameInstance.OnMiniGameWin += HandleMiniGameWin;
      _MiniGameInstance.OnMiniGameLose += HandleMiniGameLose;
      RobotEngineManager.Instance.CurrentRobot.SetIdleAnimation("NONE");
    }

    private void CloseMiniGameImmediately() {
      if (_MiniGameInstance != null) {
        DeregisterMinigameEvents();
        _MiniGameInstance.CloseMinigameImmediately();
      }
    }

    private void DeregisterMinigameEvents() {
      if (_MiniGameInstance != null) {
        _MiniGameInstance.OnMiniGameQuit -= HandleMiniGameQuit;
        _MiniGameInstance.OnMiniGameWin -= HandleMiniGameWin;
        _MiniGameInstance.OnMiniGameLose -= HandleMiniGameLose;
      }
    }

    private void CloseTimelineDialog() {
      if (_HomeViewInstance != null) {
        DeregisterDialogEvents();
        _HomeViewInstance.CloseView();
      }
    }

    private void DeregisterDialogEvents() {
      if (_HomeViewInstance != null) {
        _HomeViewInstance.OnLockedChallengeClicked -= HandleLockedChallengeClicked;
        _HomeViewInstance.OnUnlockedChallengeClicked -= HandleUnlockedChallengeClicked;
        _HomeViewInstance.OnCompletedChallengeClicked -= HandleCompletedChallengeClicked;
        _HomeViewInstance.OnEndSessionClicked -= HandleSessionEndClicked;
      }
      DailyGoalManager.Instance.MinigameConfirmed -= HandleStartChallengeRequest;
    }

    private void LoadChallengeData(ChallengeDataList sourceChallenges, 
                                   out Dictionary<string, ChallengeStatePacket> challengeStateByKey) {
      // Initial load of what's unlocked and completed from data
      challengeStateByKey = new Dictionary<string, ChallengeStatePacket>();

      // For each challenge
      ChallengeStatePacket statePacket;
      foreach (ChallengeData data in sourceChallenges.ChallengeData) {
        // Create a new data packet
        statePacket = new ChallengeStatePacket();
        statePacket.Data = data;

        // Determine the current state of the challenge
        statePacket.ChallengeUnlocked = UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value);
        // Add the challenge to the dictionary
        challengeStateByKey.Add(data.ChallengeID, statePacket);
      }
    }

    private void CompleteChallenge(CompletedChallengeData completedChallenge, bool won, StatContainer rewards) { 
      // the last session is not necessarily valid as the 'CurrentSession', as its possible
      // the day rolled over while we were playing the challenge.
      var session = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault();
      if (session != null) {
        session.Progress.Set(RobotEngineManager.Instance.CurrentRobot.GetProgressionStats());

        // TODO: This is a placeholder for rewarding green points. Eventually the daily goals system
        // will be the one responsible for rewarding green points.
        // TODO: Don't hardcode "experience"
        DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount("experience", 8);
      }
      else {
        DAS.Error(this, "Somehow managed to complete a challenge with no sessions saved!");
      }

      DataPersistenceManager.Instance.Save();
    }

    #region Testing

    public void TestLoadTimeline() {
      DestroyHubWorld();
      LoadHubWorld();
    }

    public void TestCompleteChallenge(string completedChallengeId) {
      if (_HomeViewInstance != null) {

        CompleteChallenge(new CompletedChallengeData() {
          ChallengeId = completedChallengeId 
        }, true, new StatContainer());

        // Force refresh of the dialog
        DeregisterDialogEvents();
        _HomeViewInstance.CloseViewImmediately();
        ShowTimelineDialog();
      }
    }

    #endregion
  }

  public class ChallengeStatePacket {
    public ChallengeData Data;
    public bool ChallengeUnlocked;
  }
}

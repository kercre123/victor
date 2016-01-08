using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;

namespace Cozmo.HomeHub {
  public class HomeHub : HubWorldBase {

    [SerializeField]
    private TimelineView _TimelineViewPrefab;
    private TimelineView _TimelineViewInstance;

    [SerializeField]
    private ChallengeDetailsDialog _ChallengeDetailsPrefab;
    private ChallengeDetailsDialog _ChallengeDetailsDialogInstance;

    [SerializeField]
    private ChallengeDataList _ChallengeDataList;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStatesById;

    private GameBase _MiniGameInstance;

    private string _CurrentChallengePlaying;

    public override bool LoadHubWorld() {
      LoadChallengeData(_ChallengeDataList, out _ChallengeStatesById);
      ShowTimelineDialog();
      return true;
    }

    public override bool DestroyHubWorld() {
      CloseMiniGameImmediately();

      // Deregister events
      // Destroy dialog if it exists
      if (_TimelineViewInstance != null) {
        DeregisterDialogEvents();
        _TimelineViewInstance.CloseViewImmediately();
      }
      return true;
    }

    private void ShowTimelineDialog() {
      // Create dialog with the game prefabs
      GameObject newView = GameObject.Instantiate(_TimelineViewPrefab.gameObject);
      newView.transform.position = Vector3.zero;
      newView.transform.SetParent(UIManager.GetUICanvas().transform, false);

      _TimelineViewInstance = newView.GetComponent<TimelineView>();
      _TimelineViewInstance.OnLockedChallengeClicked += HandleLockedChallengeClicked;
      _TimelineViewInstance.OnUnlockedChallengeClicked += HandleUnlockedChallengeClicked;
      _TimelineViewInstance.OnCompletedChallengeClicked += HandleCompletedChallengeClicked;

      // Show the current state of challenges being locked/unlocked
      _TimelineViewInstance.Initialize(_ChallengeStatesById);
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
      _CurrentChallengePlaying = challengeClicked;

      // Close dialog
      // TODO: Don't close the hub dialog during minigames
      CloseTimelineDialog();
      _ChallengeDetailsDialogInstance.CloseViewImmediately();

      // Play minigame immediately
      PlayMinigame(_ChallengeStatesById[challengeClicked].data);
    }

    private void HandleCompletedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      OpenChallengeDetailsDialog(challengeClicked, buttonTransform);
    }

    private void OpenChallengeDetailsDialog(string challenge, Transform buttonTransform) {
      // We need to initialize the dialog first before opening the view, so don't animate right away
      _ChallengeDetailsDialogInstance = UIManager.OpenView(_ChallengeDetailsPrefab, false) as ChallengeDetailsDialog;
      _ChallengeDetailsDialogInstance.Initialize(_ChallengeStatesById[challenge].data, buttonTransform);
      _ChallengeDetailsDialogInstance.OpenView();

      // React to when we should start the challenge.
      _ChallengeDetailsDialogInstance.ChallengeStarted += HandleStartChallengeClicked;
    }

    private void HandleMiniGameLose() {
      // Reset the current challenge
      _CurrentChallengePlaying = null;
      ShowTimelineDialog();
    }

    private void HandleMiniGameWin() {
      // If we are in a challenge that needs to be completed, complete it
      if (_CurrentChallengePlaying != null) {
        CompleteChallenge(_CurrentChallengePlaying);
        _CurrentChallengePlaying = null;
      }
      ShowTimelineDialog();
    }

    private void HandleMiniGameQuit() {
      // Reset the current challenge
      _CurrentChallengePlaying = null;
      ShowTimelineDialog();
    }

    private void PlayMinigame(ChallengeData challengeData) {
      // Reset the robot behavior
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
        RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      }

      GameObject newMiniGameObject = GameObject.Instantiate(challengeData.MinigamePrefab);
      _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
      _MiniGameInstance.InitializeMinigame(challengeData);
      _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;
      _MiniGameInstance.OnMiniGameWin += HandleMiniGameWin;
      _MiniGameInstance.OnMiniGameLose += HandleMiniGameLose;
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
      if (_TimelineViewInstance != null) {
        DeregisterDialogEvents();
        _TimelineViewInstance.CloseView();
      }
    }

    private void DeregisterDialogEvents() {
      if (_TimelineViewInstance != null) {
        _TimelineViewInstance.OnLockedChallengeClicked -= HandleLockedChallengeClicked;
        _TimelineViewInstance.OnUnlockedChallengeClicked -= HandleUnlockedChallengeClicked;
        _TimelineViewInstance.OnCompletedChallengeClicked -= HandleCompletedChallengeClicked;
      }
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
        statePacket.data = data;

        // Determine the current state of the challenge
        statePacket.currentState = DetermineCurrentChallengeState(data, DataPersistenceManager.Instance.Data.CompletedChallengeIds);
        // TODO: Set the unlock progress from persistant data (for now, use 0)
        statePacket.unlockProgress = 0;
        // Add the challenge to the dictionary
        challengeStateByKey.Add(data.ChallengeID, statePacket);
      }
    }

    private ChallengeState DetermineCurrentChallengeState(ChallengeData dataToTest, List<string> completedChallenges) {
      // If the challenge is in the completed challenges list, it has been completed
      ChallengeState challengeState = ChallengeState.LOCKED;
      if (completedChallenges.Contains(dataToTest.ChallengeID)) {
        challengeState = ChallengeState.COMPLETED;
      }
      else {
        Robot currentRobot = RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null;
        if (dataToTest.ChallengeReqs.MeetsRequirements(currentRobot, completedChallenges)) {
          // Otherwise, it is locked or unlocked based on its requirements
          // TODO: Add case for when the challenge is unlocked, but not actionable
          challengeState = ChallengeState.UNLOCKED;
        }
      }
      return challengeState;
    }

    private void CompleteChallenge(string completedChallengeId) { 
      // If the completed challenge is not already complete
      if (!DataPersistenceManager.Instance.Data.CompletedChallengeIds.Contains(completedChallengeId)) {
        // Add the current challenge to the completed list
        DataPersistenceManager.Instance.Data.CompletedChallengeIds.Add(completedChallengeId);
        _ChallengeStatesById[completedChallengeId].currentState = ChallengeState.FRESHLY_COMPLETED;

        // Check all the locked challenges to see if any new ones unlocked.
        foreach (ChallengeStatePacket challengeState in _ChallengeStatesById.Values) {
          if (challengeState.currentState == ChallengeState.LOCKED) {
            var lastState = challengeState.currentState;
            challengeState.currentState = DetermineCurrentChallengeState(challengeState.data, 
              DataPersistenceManager.Instance.Data.CompletedChallengeIds);

            if (lastState == ChallengeState.LOCKED && challengeState.currentState == ChallengeState.UNLOCKED) {
              challengeState.currentState = ChallengeState.FRESHLY_UNLOCKED;
            }
          }
        }
        DataPersistenceManager.Instance.Save();
      }
    }

    // TODO: Pragma out for production

    #region Testing

    public void TestLoadTimeline() {
      DestroyHubWorld();
      LoadHubWorld();
    }

    public void TestCompleteChallenge(string completedChallengeId) {
      if (_TimelineViewInstance != null) {
        CompleteChallenge(completedChallengeId);

        // Force refresh of the dialog
        DeregisterDialogEvents();
        _TimelineViewInstance.CloseViewImmediately();
        ShowTimelineDialog();
      }
    }

    #endregion
  }

  public class ChallengeStatePacket {
    public ChallengeData data;
    public ChallengeState currentState;
    public float unlockProgress;
  }

  public enum ChallengeState {
    LOCKED,
    UNLOCKED,
    COMPLETED,

    // these are used by the UI to know when to play animations.
    // TimelineView will then bump them into unlocked or completed
    FRESHLY_COMPLETED,
    FRESHLY_UNLOCKED
  }
}

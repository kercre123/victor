using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;

namespace Cozmo.HubWorld {
  public class HubWorld : HubWorldBase {

    [SerializeField]
    private HubWorldView _HubWorldViewPrefab;
    private HubWorldView _HubWorldViewInstance;

    [SerializeField]
    private ChallengeDetailsDialog _ChallengeDetailsPrefab;
    private ChallengeDetailsDialog _ChallengeDetailsDialogInstance;

    [SerializeField]
    private ChallengeDataList _ChallengeDataList;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStatesById;

    private GameBase _MiniGameInstance;

    private string _CurrentChallengePlaying;

    private void Awake() {
      HubWorldPane.HubWorldPaneOpened += HandleHubWorldPaneOpenHandler;
    }

    private void OnDestroy() {
      HubWorldPane.HubWorldPaneOpened -= HandleHubWorldPaneOpenHandler;
    }

    public override bool LoadHubWorld() {
      LoadChallengeData(_ChallengeDataList, out _ChallengeStatesById);
      ShowHubWorldDialog();
      return true;
    }

    public override bool DestroyHubWorld() {
      CloseMiniGameImmediately();

      // Deregister events
      // Destroy dialog if it exists
      if (_HubWorldViewInstance != null) {
        DeregisterDialogEvents();
        _HubWorldViewInstance.CloseViewImmediately();
      }
      return true;
    }

    private void ShowHubWorldDialog() {
      // Create dialog with the game prefabs

      GameObject newView = GameObject.Instantiate(_HubWorldViewPrefab.gameObject);
      newView.transform.position = Vector3.zero;

      _HubWorldViewInstance = newView.GetComponent<HubWorldView>();
      _HubWorldViewInstance.OnLockedChallengeClicked += HandleLockedChallengeClicked;
      _HubWorldViewInstance.OnUnlockedChallengeClicked += HandleUnlockedChallengeClicked;
      _HubWorldViewInstance.OnCompletedChallengeClicked += HandleCompletedChallengeClicked;

      // Show the current state of challenges being locked/unlocked
      _HubWorldViewInstance.Initialize(_ChallengeStatesById);
    }

    private void HandleLockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      // Do nothing for now
    }

    private void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      // Show some details about the challenge before starting it
      // We need to initialize the dialog first before opening the view, so don't use UIManager's OpenView method.
      GameObject dialogObject = UIManager.CreateUIElement(_ChallengeDetailsPrefab.gameObject);
      _ChallengeDetailsDialogInstance = dialogObject.GetComponent<ChallengeDetailsDialog>();
      _ChallengeDetailsDialogInstance.Initialize(_ChallengeStatesById[challengeClicked].data, buttonTransform);
      _ChallengeDetailsDialogInstance.OpenView();

      // React to when we should start the challenge.
      _ChallengeDetailsDialogInstance.ChallengeStarted += HandleStartChallengeClicked;
    }

    private void HandleStartChallengeClicked(string challengeClicked) {
      _ChallengeDetailsDialogInstance.ChallengeStarted -= HandleStartChallengeClicked;

      // Keep track of the current challenge
      _CurrentChallengePlaying = challengeClicked;

      // Close dialog
      // TODO: Don't close the hub dialog during minigames
      CloseHubWorldDialog();
      _ChallengeDetailsDialogInstance.CloseViewImmediately();
      HubWorldCamera.Instance.ReturnCameraToDefault();

      // Play minigame immediately
      PlayMinigame(_ChallengeStatesById[challengeClicked].data);
    }

    private void HandleCompletedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      // Show some details about the challenge before starting it
      // We need to initialize the dialog first before opening the view, so don't use UIManager's OpenView method.
      GameObject dialogObject = UIManager.CreateUIElement(_ChallengeDetailsPrefab.gameObject);
      _ChallengeDetailsDialogInstance = dialogObject.GetComponent<ChallengeDetailsDialog>();
      _ChallengeDetailsDialogInstance.Initialize(_ChallengeStatesById[challengeClicked].data, buttonTransform);
      _ChallengeDetailsDialogInstance.OpenView();

      // React to when we should start the challenge.
      _ChallengeDetailsDialogInstance.ChallengeStarted += HandleStartChallengeClicked;
    }

    private void HandleMiniGameLose() {
      // Reset the current challenge
      _CurrentChallengePlaying = null;
      ShowHubWorldDialog();
    }

    private void HandleMiniGameWin() {
      // If we are in a challenge that needs to be completed, complete it
      if (_CurrentChallengePlaying != null) {
        CompleteChallenge(_CurrentChallengePlaying);
        _CurrentChallengePlaying = null;
      }
      ShowHubWorldDialog();
    }

    private void HandleMiniGameQuit() {
      // Reset the current challenge
      _CurrentChallengePlaying = null;
      ShowHubWorldDialog();
    }

    private void PlayMinigame(ChallengeData challengeData) {
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

    private void CloseHubWorldDialog() {
      if (_HubWorldViewInstance != null) {
        DeregisterDialogEvents();
        _HubWorldViewInstance.CloseView();
      }
    }

    private void DeregisterDialogEvents() {
      if (_HubWorldViewInstance != null) {
        _HubWorldViewInstance.OnLockedChallengeClicked -= HandleLockedChallengeClicked;
        _HubWorldViewInstance.OnUnlockedChallengeClicked -= HandleUnlockedChallengeClicked;
        _HubWorldViewInstance.OnCompletedChallengeClicked -= HandleCompletedChallengeClicked;
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

    private void HandleHubWorldPaneOpenHandler(HubWorldPane hubWorldPane) {
      hubWorldPane.Initialize(this);
    }

    public void TestLoadHubWorld() {
      DestroyHubWorld();
      LoadHubWorld();
    }

    public void TestCompleteChallenge(string completedChallengeId) {
      if (_HubWorldViewInstance != null) {
        CompleteChallenge(completedChallengeId);

        // Force refresh of the dialog
        DeregisterDialogEvents();
        _HubWorldViewInstance.CloseViewImmediately();
        ShowHubWorldDialog();
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
    // HubWorldView will then bump them into unlocked or completed
    FRESHLY_COMPLETED,
    FRESHLY_UNLOCKED
  }
}

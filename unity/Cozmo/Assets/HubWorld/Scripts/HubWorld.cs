using UnityEngine;
using System.Collections;
using System.Collections.Generic;


public class HubWorld : HubWorldBase {

  [SerializeField]
  private HubWorldView _HubWorldDialogPrefab;
  private HubWorldView _HubWorldDialogInstance;

  private GameBase _MiniGameInstance;

  [SerializeField]
  private ChallengeDataList _ChallengeDataList;

  private Dictionary<string, ChallengeStatePacket> _ChallengeStatesById;
  private List<string> _CompletedChallengeIds;

  private string _CurrentChallengePlaying;

  private void Awake() {
    HubWorldPane.HubWorldPaneOpened += HandleHubWorldPaneOpenHandler;
  }

  public override bool LoadHubWorld() {
    LoadChallengeData(_ChallengeDataList, out _ChallengeStatesById);
    ShowHubWorldDialog();
    return true;
  }

  public override bool DestroyHubWorld() {
    CloseMiniGame();

    // Deregister events
    // Destroy dialog if it exists
    if (_HubWorldDialogInstance != null) {
      DeregisterDialogEvents();
      _HubWorldDialogInstance.CloseViewImmediately();
    }

    HubWorldPane.HubWorldPaneOpened -= HandleHubWorldPaneOpenHandler;
    return true;
  }

  private void ShowHubWorldDialog() {
    // Create dialog with the game prefabs

    GameObject newView = GameObject.Instantiate(_HubWorldDialogPrefab.gameObject);
    newView.transform.position = Vector3.zero;

    _HubWorldDialogInstance = newView.GetComponent<HubWorldView>();
    _HubWorldDialogInstance.OnLockedChallengeClicked += HandleLockedChallengeClicked;
    _HubWorldDialogInstance.OnUnlockedChallengeClicked += HandleUnlockedChallengeClicked;
    _HubWorldDialogInstance.OnCompletedChallengeClicked += HandleCompletedChallengeClicked;

    // Show the current state of challenges being locked/unlocked
    _HubWorldDialogInstance.Initialize(_ChallengeStatesById);
  }

  private void HandleLockedChallengeClicked(string challengeClicked) {
    // Do nothing for now
  }

  private void HandleUnlockedChallengeClicked(string challengeClicked) {
    // Keep track of the current challenge
    _CurrentChallengePlaying = challengeClicked;

    // Close dialog
    CloseHubWorldDialog();

    // Play minigame immediately
    PlayMinigame(_ChallengeStatesById[challengeClicked].data);

    // TODO: Show panel instead of playing minigame right away
    // TODO: Don't close the hub dialog during minigames
  }

  private void HandleCompletedChallengeClicked(string challengeClicked) {
    // For now, play the game but don't increase progress when you win
    CloseHubWorldDialog();
    PlayMinigame(_ChallengeStatesById[challengeClicked].data);
  }

  private void HandleMiniGameLose() {
    // Reset the current challenge
    _CurrentChallengePlaying = null;

    CloseMiniGame();
    ShowHubWorldDialog();
  }

  private void HandleMiniGameWin() {
    // If we are in a challenge that needs to be completed, complete it
    if (_CurrentChallengePlaying != null) {
      CompleteChallenge(_CurrentChallengePlaying);
      _CurrentChallengePlaying = null;
    }

    CloseMiniGame();
    ShowHubWorldDialog();
  }

  private void HandleMiniGameQuit() {
    // Reset the current challenge
    _CurrentChallengePlaying = null;

    CloseMiniGame();
    ShowHubWorldDialog();
  }

  private void PlayMinigame(ChallengeData challengeData) {
    GameObject newMiniGameObject = GameObject.Instantiate(challengeData.MinigamePrefab);
    _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
    _MiniGameInstance.LoadMinigameConfig(challengeData.MinigameConfig);
    _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;
    _MiniGameInstance.OnMiniGameWin += HandleMiniGameWin;
    _MiniGameInstance.OnMiniGameLose += HandleMiniGameLose;
  }

  private void CloseMiniGame() {
    // Destroy game if it exists
    if (_MiniGameInstance != null) {
      _MiniGameInstance.OnMiniGameQuit -= HandleMiniGameQuit;
      _MiniGameInstance.OnMiniGameWin -= HandleMiniGameWin;
      _MiniGameInstance.OnMiniGameLose -= HandleMiniGameLose;
      
      _MiniGameInstance.CleanUp();
      Destroy(_MiniGameInstance.gameObject);
    }
  }

  private void CloseHubWorldDialog() {
    if (_HubWorldDialogInstance != null) {
      DeregisterDialogEvents();
      _HubWorldDialogInstance.CloseView();
    }
  }

  private void DeregisterDialogEvents() {
    if (_HubWorldDialogInstance != null) {
      _HubWorldDialogInstance.OnLockedChallengeClicked -= HandleLockedChallengeClicked;
      _HubWorldDialogInstance.OnUnlockedChallengeClicked -= HandleUnlockedChallengeClicked;
      _HubWorldDialogInstance.OnCompletedChallengeClicked -= HandleCompletedChallengeClicked;
    }
  }

  private void LoadChallengeData(ChallengeDataList sourceChallenges, 
                                 out Dictionary<string, ChallengeStatePacket> challengeStateByKey) {
    // Initial load of what's unlocked and completed from data
    challengeStateByKey = new Dictionary<string, ChallengeStatePacket>();

    // TODO: Get the list of completed challenges from data (for now, use empty list)
    _CompletedChallengeIds = new List<string>();

    // For each challenge
    ChallengeStatePacket statePacket;
    foreach (ChallengeData data in sourceChallenges.ChallengeData) {
      // Create a new data packet
      statePacket = new ChallengeStatePacket();
      statePacket.data = data;

      // Determine the current state of the challenge
      statePacket.currentState = DetermineCurrentChallengeState(data, _CompletedChallengeIds);
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
    if (!_CompletedChallengeIds.Contains(completedChallengeId)) {
      // Add the current challenge to the completed list
      _CompletedChallengeIds.Add(completedChallengeId);
      _ChallengeStatesById[completedChallengeId].currentState = ChallengeState.COMPLETED;

      // Check all the locked challenges to see if any new ones unlocked.
      foreach (ChallengeStatePacket challengeState in _ChallengeStatesById.Values) {
        if (challengeState.currentState == ChallengeState.LOCKED) {
          challengeState.currentState = DetermineCurrentChallengeState(challengeState.data, _CompletedChallengeIds);
        }
      }
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
    if (_HubWorldDialogInstance != null) {
      CompleteChallenge(completedChallengeId);

      // Force refresh of the dialog
      DeregisterDialogEvents();
      _HubWorldDialogInstance.CloseViewImmediately();
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
  COMPLETED
}

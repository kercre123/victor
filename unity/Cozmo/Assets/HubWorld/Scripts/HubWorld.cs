using UnityEngine;
using System.Collections;
using System.Collections.Generic;


public class HubWorld : HubWorldBase {

  [SerializeField]
  private HubWorldDialog _HubWorldDialogPrefab;
  private HubWorldDialog _HubWorldDialogInstance;

  private GameBase _MiniGameInstance;

  [SerializeField]
  private TextAsset _TempLevelAsset;

  [SerializeField]
  private ChallengeDataList _ChallengeDataList;

  private Dictionary<string, ChallengeStatePacket> _ChallengeStatesById;
  private List<string> _CompletedChallengeIds;

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
      _HubWorldDialogInstance.CloseDialogImmediately();
    }

    HubWorldPane.HubWorldPaneOpened -= HandleHubWorldPaneOpenHandler;
    return true;
  }

  private void ShowHubWorldDialog() {
    // Create dialog with the game prefabs
    _HubWorldDialogInstance = UIManager.OpenDialog(_HubWorldDialogPrefab) as HubWorldDialog;
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
    // TODO: Keep track of the current challenge

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
    // TODO: Reset the current challenge

    CloseMiniGame();
    ShowHubWorldDialog();
  }

  private void HandleMiniGameWin() {
    // TODO: If we are in a challenge that needs to be completed, complete it

    CloseMiniGame();
    ShowHubWorldDialog();
  }

  private void HandleMiniGameQuit() {
    // TODO: Reset the current challenge

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
      _HubWorldDialogInstance.CloseDialog();
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
    // TODO: STUB
    // Initial load of what's unlocked and completed from data
    challengeStateByKey = new Dictionary<string, ChallengeStatePacket>();

    // TODO: Get the list of completed challenges from data (for now, use empty list)

    // For each challenge
    // Create a new data packet
    // Determine the current state of the challenge
    // TODO: Set the unlock progress from persistant data (for now, use 0)
    // Add the challenge to the dictionary
  }

  private ChallengeState DetermineCurrentChallengeState(ChallengeData dataToTest, List<string> completedChallenges) {
    // If the challenge is in the completed challenges list, it has been completed
    // Otherwise, it is locked or unlocked based on its requirements
    // TODO: Add case for when the challenge is unlocked, but not actionable
    return ChallengeState.LOCKED;
  }

  private void CompleteChallenge(string completedChallengeId) { 
    // If the completed challenge is not already complete
    // TODO: Add the current challenge to the completed list
    // TODO: Check all the locked challenges to see if any new ones unlocked.
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
      _HubWorldDialogInstance.CloseDialogImmediately();
      ShowHubWorldDialog();
    }
  }

  #endregion
}

public struct ChallengeStatePacket {
  public ChallengeData data;
  public ChallengeState currentState;
  public float unlockProgress;
}

public enum ChallengeState {
  LOCKED,
  UNLOCKED,
  COMPLETED
}

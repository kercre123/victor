using Anki.Assets;
using Anki.Cozmo;
using Cozmo.RequestGame;
using DataPersistence;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.Challenge {
  public class ChallengeManager {

    public delegate void ShowEndGameDialogHandler();
    public event ShowEndGameDialogHandler OnShowEndGameDialog;

    public delegate void ChallengeFinishedLoadingHandler(AnimationTrigger getOutAnim);
    public event ChallengeFinishedLoadingHandler OnChallengeFinishedLoading;

    public delegate void ChallengeViewFinishedClosingHandler();
    public event ChallengeViewFinishedClosingHandler OnChallengeViewFinishedClosing;

    public delegate void ChallengeCompletedHandler();
    public event ChallengeCompletedHandler OnChallengeCompleted;

    public class ChallengeStatePacket {
      public ChallengeData Data;
      public bool IsChallengeUnlocked;
    }

    private SerializableAssetBundleNames _ChallengeDataPrefabAssetBundle;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStatesById;

    private CompletedChallengeData _CurrentChallengePlaying;

    private bool _CurrentChallengeWasRequest;

    private GameBase _ChallengeInstance;
    public GameBase ChallengeInstance { get { return _ChallengeInstance; } }

    public bool IsChallengePlaying { get { return _ChallengeInstance != null; } }

    public ChallengeManager(SerializableAssetBundleNames challengeDataPrefabAssetBundle) {
      _ChallengeDataPrefabAssetBundle = challengeDataPrefabAssetBundle;
      LoadChallengeData(ChallengeDataList.Instance, out _ChallengeStatesById);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshUnlockInfo);
    }

    public void CleanUp() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshUnlockInfo);
      CloseChallengeImmediately();
    }

    public List<ChallengeStatePacket> GetActivities() {
      List<ChallengeStatePacket> activitiesData = new List<ChallengeStatePacket>();
      foreach (var kvp in _ChallengeStatesById) {
        if (kvp.Value.Data.IsActivity) {
          activitiesData.Add(kvp.Value);
        }
      }
      activitiesData.Sort((x, y) => {
        return x.Data.ActivityData.ActivityPriority.CompareTo(y.Data.ActivityData.ActivityPriority);
      });
      return activitiesData;
    }

    #region Unlock State Update

    private void RefreshUnlockInfo(object message) {
      LoadChallengeData(ChallengeDataList.Instance, out _ChallengeStatesById);
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
        statePacket.IsChallengeUnlocked = UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value);
        // Add the challenge to the dictionary
        challengeStateByKey.Add(data.ChallengeID, statePacket);
      }
    }

    #endregion

    #region Challenge Enter

    public void SetCurrentChallenge(string challengeId, bool wasRequest) {
      // Keep track of the current challenge
      _CurrentChallengePlaying = new CompletedChallengeData() {
        ChallengeId = challengeId,
        StartTime = System.DateTime.UtcNow,
      };
      _CurrentChallengeWasRequest = wasRequest;
    }

    public RequestGameConfig GetCurrentRequestGameConfig() {
      RequestGameConfig rc = null;
      if (_CurrentChallengePlaying != null && _CurrentChallengeWasRequest) {
        RequestGameListConfig rcList = RequestGameListConfig.Instance;
        for (int i = 0; i < rcList.RequestList.Length; i++) {
          if (rcList.RequestList[i].ChallengeID == _CurrentChallengePlaying.ChallengeId) {
            rc = rcList.RequestList[i];
            break;
          }
        }
      }
      return rc;
    }

    public bool LoadChallenge() {
      if (_CurrentChallengePlaying == null) {
        DAS.Error("ChallengeManager.LoadChallenge.CurrentChallengeNull", "Call SetCurrentChallenge before calling LoadChallenge");
        return false;
      }

      GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnChallengeStarted,
                                                                             _CurrentChallengePlaying.ChallengeId,
                                                                             _CurrentChallengeWasRequest));
      LoadChallengeAssetBundle();
      return true;
    }

    private void LoadChallengeAssetBundle() {
      AssetBundleManager.Instance.LoadAssetBundleAsync(
        _ChallengeDataPrefabAssetBundle.Value.ToString(), (bool prefabsSuccess) => {
          if (prefabsSuccess) {

            LoadChallengeInternal(_ChallengeStatesById[_CurrentChallengePlaying.ChallengeId].Data);
          }
          else {
            // TODO show error dialog and boot to home
            DAS.Error("HomeHub.HandleHomeViewCloseAnimationFinished", "Failed to load asset bundle " + _ChallengeDataPrefabAssetBundle.Value.ToString());
          }
        });
    }

    private void LoadChallengeInternal(ChallengeData challengeData) {
      challengeData.LoadPrefabData((ChallengePrefabData prefabData) => {
        GameObject newChallengeObject = GameObject.Instantiate(prefabData.ChallengePrefab);
        _ChallengeInstance = newChallengeObject.GetComponent<GameBase>();

        // OnSharedChallengeViewInitialized is called as part of the InitializeChallenge flow; 
        // On device this involves loading assets from data but in editor it may be instantaneous
        // so we need to listen to the event first and then initialize
        _ChallengeInstance.OnSharedMinigameViewInitialized += HandleSharedMinigameViewInitialized;
        _ChallengeInstance.InitializeChallenge(challengeData);

        _ChallengeInstance.OnShowEndGameDialog += HandleEndGameDialog;
        _ChallengeInstance.OnChallengeWin += HandleChallengeWin;
        _ChallengeInstance.OnChallengeLose += HandleChallengeLose;
        _ChallengeInstance.OnChallengeQuit += HandleChallengeQuit;

        // Set the GetOut animation to play when this challenge is destroyed
        if (OnChallengeFinishedLoading != null) {
          OnChallengeFinishedLoading(challengeData.GetOutAnimTrigger.Value);
        }
      });
    }

    private void HandleSharedMinigameViewInitialized(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      _ChallengeInstance.OnSharedMinigameViewInitialized -= HandleSharedMinigameViewInitialized;
      newView.DialogCloseAnimationFinished += HandleChallengeFinishedClosing;
    }

    #endregion

    #region Challenge Exit 

    private void HandleEndGameDialog() {
      if (OnShowEndGameDialog != null) {
        OnShowEndGameDialog();
      }
    }

    private void HandleChallengeWin() {
      HandleChallengeCompleted(didWin: true);
    }

    private void HandleChallengeLose() {
      HandleChallengeCompleted(didWin: false);
    }

    private void HandleChallengeCompleted(bool didWin) {
      // If we are in a challenge that needs to be completed, complete it
      if (_CurrentChallengePlaying != null) {
        if (OnChallengeCompleted != null) {
          OnChallengeCompleted();
        }
        HandleChallengeQuit();
      }
    }

    private void HandleChallengeQuit() {
      // Reset the current challenge and re-register the HandleStartChallengeRequest
      _CurrentChallengePlaying = null;
    }

    private void HandleChallengeFinishedClosing() {
      _ChallengeInstance.SharedMinigameView.DialogCloseAnimationFinished -= HandleChallengeFinishedClosing;
      UnloadChallengeAssetBundle();
      if (OnChallengeViewFinishedClosing != null) {
        OnChallengeViewFinishedClosing();
      }
    }

    public void CloseChallengeImmediately() {
      if (_ChallengeInstance != null) {
        DeregisterChallengeEvents();
        _ChallengeInstance.CloseChallengeImmediately();
        _ChallengeInstance = null;
        UnloadChallengeAssetBundle();
      }
    }

    private void DeregisterChallengeEvents() {
      if (_ChallengeInstance != null) {
        _ChallengeInstance.OnChallengeQuit -= HandleChallengeQuit;
        _ChallengeInstance.OnChallengeWin -= HandleChallengeWin;
        _ChallengeInstance.OnChallengeLose -= HandleChallengeLose;
        _ChallengeInstance.OnShowEndGameDialog -= HandleEndGameDialog;
        _ChallengeInstance.OnSharedMinigameViewInitialized -= HandleSharedMinigameViewInitialized;
        if (_ChallengeInstance.SharedMinigameView != null) {
          _ChallengeInstance.SharedMinigameView.DialogCloseAnimationFinished -= HandleChallengeFinishedClosing;
        }
      }
    }

    private void UnloadChallengeAssetBundle() {
      AssetBundleManager.Instance.UnloadAssetBundle(_ChallengeDataPrefabAssetBundle.Value.ToString());
    }
    #endregion

    #region Temp Game Selection

    public string GetRandomChallenge() {
      int randomIndex = Random.Range(0, _ChallengeStatesById.Keys.Count);
      string[] challengeIds = new string[_ChallengeStatesById.Keys.Count];
      _ChallengeStatesById.Keys.CopyTo(challengeIds, 0);
      return challengeIds[randomIndex];
    }

    #endregion
  }
}
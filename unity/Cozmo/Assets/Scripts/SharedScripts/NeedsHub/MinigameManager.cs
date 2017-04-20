using Anki.Assets;
using Anki.Cozmo;
using Cozmo.RequestGame;
using DataPersistence;
using UnityEngine;
using System.Collections.Generic;

namespace Cozmo.Minigame {
  public class MinigameManager {

    public delegate void ShowEndGameDialogHandler();
    public event ShowEndGameDialogHandler OnShowEndGameDialog;

    public delegate void MinigameFinishedLoadingHandler(AnimationTrigger getOutAnim);
    public event MinigameFinishedLoadingHandler OnMinigameFinishedLoading;

    public delegate void MinigameViewFinishedClosingHandler();
    public event MinigameViewFinishedClosingHandler OnMinigameViewFinishedClosing;

    public delegate void MinigameCompletedHandler();
    public event MinigameCompletedHandler OnMinigameCompleted;

    private class MinigameStatePacket {
      public ChallengeData Data;
      public bool IsMinigameUnlocked;
    }

    private SerializableAssetBundleNames _MinigameDataPrefabAssetBundle;

    private Dictionary<string, MinigameStatePacket> _MinigameStatesById;

    private CompletedChallengeData _CurrentMinigamePlaying;

    private bool _CurrentMinigameWasRequest;

    private GameBase _MiniGameInstance;
    public GameBase MinigameInstance { get { return _MiniGameInstance; } }

    public bool IsMinigamePlaying { get { return _MiniGameInstance != null; } }

    public MinigameManager(SerializableAssetBundleNames minigameDataPrefabAssetBundle) {
      _MinigameDataPrefabAssetBundle = minigameDataPrefabAssetBundle;
      LoadChallengeData(ChallengeDataList.Instance, out _MinigameStatesById);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshUnlockInfo);
    }

    public void CleanUp() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshUnlockInfo);
      CloseMiniGameImmediately();
    }

    private void LoadChallengeData(ChallengeDataList sourceChallenges,
                                   out Dictionary<string, MinigameStatePacket> minigameStateByKey) {
      // Initial load of what's unlocked and completed from data
      minigameStateByKey = new Dictionary<string, MinigameStatePacket>();

      // For each challenge
      MinigameStatePacket statePacket;
      foreach (ChallengeData data in sourceChallenges.ChallengeData) {
        // Create a new data packet
        statePacket = new MinigameStatePacket();
        statePacket.Data = data;

        // Determine the current state of the challenge
        statePacket.IsMinigameUnlocked = UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value);
        // Add the challenge to the dictionary
        minigameStateByKey.Add(data.ChallengeID, statePacket);
      }
    }

    public string GetRandomMinigame() {
      int randomIndex = Random.Range(0, _MinigameStatesById.Keys.Count);
      string[] minigameIds = new string[_MinigameStatesById.Keys.Count];
      _MinigameStatesById.Keys.CopyTo(minigameIds, 0);
      return minigameIds[randomIndex];
    }

    public void SetCurrentMinigame(string minigameId, bool wasRequest) {
      // Keep track of the current challenge
      _CurrentMinigamePlaying = new CompletedChallengeData() {
        ChallengeId = minigameId,
        StartTime = System.DateTime.UtcNow,
      };
      _CurrentMinigameWasRequest = wasRequest;
    }

    public RequestGameConfig GetCurrentRequestGameConfig() {
      RequestGameConfig rc = null;
      if (_CurrentMinigamePlaying != null && _CurrentMinigameWasRequest) {
        RequestGameListConfig rcList = RequestGameListConfig.Instance;
        for (int i = 0; i < rcList.RequestList.Length; i++) {
          if (rcList.RequestList[i].ChallengeID == _CurrentMinigamePlaying.ChallengeId) {
            rc = rcList.RequestList[i];
            break;
          }
        }
      }
      return rc;
    }

    public bool LoadMinigame() {
      if (_CurrentMinigamePlaying == null) {
        DAS.Error("MinigameManager.LoadMinigame.CurrentMinigameNull", "Call SetCurrentChallenge before calling LoadMinigame");
        return false;
      }

      GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnChallengeStarted,
                                                                             _CurrentMinigamePlaying.ChallengeId,
                                                                             _CurrentMinigameWasRequest));
      LoadMinigameAssetBundle();
      return true;
    }

    private void LoadMinigameAssetBundle() {
      AssetBundleManager.Instance.LoadAssetBundleAsync(
        _MinigameDataPrefabAssetBundle.Value.ToString(), (bool prefabsSuccess) => {
          if (prefabsSuccess) {

            LoadMinigameInternal(_MinigameStatesById[_CurrentMinigamePlaying.ChallengeId].Data);
          }
          else {
            // TODO show error dialog and boot to home
            DAS.Error("HomeHub.HandleHomeViewCloseAnimationFinished", "Failed to load asset bundle " + _MinigameDataPrefabAssetBundle.Value.ToString());
          }
        });
    }

    private void LoadMinigameInternal(ChallengeData challengeData) {
      challengeData.LoadPrefabData((ChallengePrefabData prefabData) => {
        GameObject newMiniGameObject = GameObject.Instantiate(prefabData.MinigamePrefab);
        _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();

        // OnSharedMinigameViewInitialized is called as part of the InitializeMinigame flow; 
        // On device this involves loading assets from data but in editor it may be instantaneous
        // so we need to listen to the event first and then initialize
        _MiniGameInstance.OnSharedMinigameViewInitialized += HandleSharedMinigameViewInitialized;
        _MiniGameInstance.InitializeMinigame(challengeData);

        _MiniGameInstance.OnShowEndGameDialog += HandleEndGameDialog;
        _MiniGameInstance.OnMiniGameWin += HandleMiniGameWin;
        _MiniGameInstance.OnMiniGameLose += HandleMiniGameLose;
        _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;

        // Set the GetOut animation to play when this minigame is destroyed
        if (OnMinigameFinishedLoading != null) {
          OnMinigameFinishedLoading(challengeData.GetOutAnimTrigger.Value);
        }
      });
    }

    private void RefreshUnlockInfo(object message) {
      LoadChallengeData(ChallengeDataList.Instance, out _MinigameStatesById);
    }

    private void HandleSharedMinigameViewInitialized(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      _MiniGameInstance.OnSharedMinigameViewInitialized -= HandleSharedMinigameViewInitialized;
      newView.DialogCloseAnimationFinished += HandleMinigameFinishedClosing;
    }

    private void HandleEndGameDialog() {
      if (OnShowEndGameDialog != null) {
        OnShowEndGameDialog();
      }
    }

    private void HandleMiniGameWin() {
      HandleMiniGameCompleted(didWin: true);
    }

    private void HandleMiniGameLose() {
      HandleMiniGameCompleted(didWin: false);
    }

    private void HandleMiniGameCompleted(bool didWin) {
      // If we are in a challenge that needs to be completed, complete it
      if (_CurrentMinigamePlaying != null) {
        if (OnMinigameCompleted != null) {
          OnMinigameCompleted();
        }
        HandleMiniGameQuit();
      }
    }

    private void HandleMiniGameQuit() {
      // Reset the current challenge and re-register the HandleStartChallengeRequest
      _CurrentMinigamePlaying = null;
    }

    private void HandleMinigameFinishedClosing() {
      _MiniGameInstance.SharedMinigameView.DialogCloseAnimationFinished -= HandleMinigameFinishedClosing;
      UnloadMinigameAssetBundle();
      if (OnMinigameViewFinishedClosing != null) {
        OnMinigameViewFinishedClosing();
      }
    }

    public void CloseMiniGameImmediately() {
      if (_MiniGameInstance != null) {
        DeregisterMinigameEvents();
        _MiniGameInstance.CloseMinigameImmediately();
        _MiniGameInstance = null;
        UnloadMinigameAssetBundle();
      }
    }

    private void DeregisterMinigameEvents() {
      if (_MiniGameInstance != null) {
        _MiniGameInstance.OnMiniGameQuit -= HandleMiniGameQuit;
        _MiniGameInstance.OnMiniGameWin -= HandleMiniGameWin;
        _MiniGameInstance.OnMiniGameLose -= HandleMiniGameLose;
        _MiniGameInstance.OnShowEndGameDialog -= HandleEndGameDialog;
        _MiniGameInstance.OnSharedMinigameViewInitialized -= HandleSharedMinigameViewInitialized;
        if (_MiniGameInstance.SharedMinigameView != null) {
          _MiniGameInstance.SharedMinigameView.DialogCloseAnimationFinished -= HandleMinigameFinishedClosing;
        }
      }
    }

    private void UnloadMinigameAssetBundle() {
      AssetBundleManager.Instance.UnloadAssetBundle(_MinigameDataPrefabAssetBundle.Value.ToString());
    }
  }
}
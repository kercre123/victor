using System;
using UnityEngine;
using DataPersistence;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;
using Anki.Cozmo;

/// <summary>
/// Daily goal manager. Loads in DailyGoal information and manages 
/// </summary>
public class DailyGoalManager : MonoBehaviour {

  // TODO: Load from JSON and set up this list. Reference this list to generate
  // Daily Goals.
  private List<DailyGoal> _DailyGoalList;

  #region constants

  private const float _kMinigameNeedMin = 0.3f;
  private const float _kMinigameNeedMax = 1.0f;

  #endregion

  public Action<string> MinigameConfirmed;

  [SerializeField]
  private ChallengeDataList _ChallengeList;

  private AlertView _RequestDialog = null;
  private bool _RequestPending = false;

  // The Last Challenge ID Cozmo has requested to play
  private ChallengeData _LastChallengeData;

  #region FriendshipProgression and DailyGoals



  // Config file for friendship progression and daily goal generation
  [SerializeField]
  private DailyGoalGenerationConfig _DailyGoalGenConfig;

  public DailyGoalGenerationConfig GetDailyGoalGenConfig() {
    return _DailyGoalGenConfig;
  }

  public float GetTodayProgress() {
    if (DataPersistenceManager.Instance.CurrentSession != null) {
      return DataPersistenceManager.Instance.CurrentSession.GetTotalProgress();
    }
    else {
      return 0.0f;
    }
  }

  [SerializeField]
  private RequestGameListConfig _RequestMinigameConfig;

  public RequestGameListConfig GetRequestMinigameConfig() {
    return _RequestMinigameConfig;
  }

  public ChallengeData PickMiniGameToRequest() {
    RequestGameConfig config = _RequestMinigameConfig.RequestList[UnityEngine.Random.Range(0, _RequestMinigameConfig.RequestList.Length)];
    for (int i = 0; i < _ChallengeList.ChallengeData.Length; i++) {
      if (_ChallengeList.ChallengeData[i].ChallengeID == config.ChallengeID) {
        _LastChallengeData = _ChallengeList.ChallengeData[i];
        BehaviorGroup bGroup = GetRequestBehaviorGroup(_LastChallengeData.ChallengeID);
        // Do not reate the minigame message if the behavior group is invalid.
        if (bGroup == BehaviorGroup.MiniGame) {
          return null;
        }
        DisableRequestGameBehaviorGroups();
        RobotEngineManager.Instance.CurrentRobot.SetEnableBehaviorGroup(bGroup, true);
        return _LastChallengeData;
      }
    }
    DAS.Error(this, string.Format("Challenge ID not found in ChallengeList {0}", config.ChallengeID));
    return null;
  }

  public BehaviorGroup GetRequestBehaviorGroup(string challengeID) {
    for (int i = 0; i < _RequestMinigameConfig.RequestList.Length; i++) {
      if (challengeID == _RequestMinigameConfig.RequestList[i].ChallengeID) {
        return _RequestMinigameConfig.RequestList[i].RequestBehaviorGroup;
      }
    }
    DAS.Error(this, string.Format("Challenge ID not found in RequestMinigameList {0}", challengeID));
    return BehaviorGroup.MiniGame;
  }

  public void DisableRequestGameBehaviorGroups() {
    RobotEngineManager.Instance.CurrentRobot.SetEnableBehaviorGroup(BehaviorGroup.MiniGame, false);
    RobotEngineManager.Instance.CurrentRobot.SetEnableBehaviorGroup(BehaviorGroup.RequestSpeedTap, false);
    RobotEngineManager.Instance.CurrentRobot.SetEnableBehaviorGroup(BehaviorGroup.RequestCubePounce, false);
    RobotEngineManager.Instance.CurrentRobot.SetEnableBehaviorGroup(BehaviorGroup.RequestSimon, false);
  }

  /// <summary>
  ///  Returns a value between -1 and 1 based on how close AND far you are from completing daily goal
  /// </summary>
  /// <returns>The minigame need.</returns>
  public void SetMinigameNeed() {
    // Calculate how far you are from 50% complete
    float prog = (GetTodayProgress() * 2.0f) - 1.0f;
    // Min desire to play if daily goals are complete
    if (prog >= _kMinigameNeedMax) {
      prog = 0.0f;
    }
    prog = Math.Abs(prog);
    prog = Mathf.Lerp(_kMinigameNeedMin, _kMinigameNeedMax, prog);
    PickMiniGameToRequest();
    RobotEngineManager.Instance.CurrentRobot.SetEmotion(EmotionType.WantToPlay, prog);
  }

  #endregion

  private static DailyGoalManager _Instance = null;

  public static DailyGoalManager Instance {
    get {
      if (_Instance == null) {
        DAS.Error("DailyGoalManager", "Don't reference DailyGoalManager before Start");
      }
      return _Instance;
    }
    set {
      if (_Instance != null) {
        DAS.Error("DailyGoalManager", "There shouldn't be more than one DailyGoalManager");
      }
      _Instance = value;
    }
  }

  private void Start() {
    Instance = this;
    RobotEngineManager.Instance.OnRequestGameStart += HandleAskForMinigame;
    RobotEngineManager.Instance.OnDenyGameStart += HandleExternalRejection;
    LoadDailyGoalData();
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.OnRequestGameStart -= HandleAskForMinigame;
    RobotEngineManager.Instance.OnDenyGameStart -= HandleExternalRejection;
  }

  private void LoadDailyGoalData() {
    //string toLoad = _DailyGoalGenConfig.DailyGoalFileName;
    //_DailyGoalList = new List<DailyGoal>();
    // TODO : Deserialize here, add each to the DailyGoalList
  }

  public List<DailyGoal> GenerateDailyGoals() {
    List<DailyGoal> newGoals = new List<DailyGoal>();
    // TODO: Properly Generate Daily Goals based on information loaded from tool.

    SendDasEventsForGoalGeneration(newGoals);
    return newGoals;
  }

  private void SendDasEventsForGoalGeneration(List<DailyGoal> goals) {
    if (goals.Count > 0) {
      for (int i = 0; i < goals.Count; i++) {
        DAS.Event(DASConstants.Goal.kGeneration, DASUtil.FormatDate(DataPersistenceManager.Today), 
          new Dictionary<string,string> { {
              "$data",
              DASUtil.FormatGoal(goals[i])
            }
          });
      }
    }
  }

  private void HandleAskForMinigame(Anki.Cozmo.ExternalInterface.RequestGameStart message) {
    if (_RequestDialog != null) {
      // Avoid dupes
      return;
    }
   
    ChallengeData data = _LastChallengeData;
    // Do not send the minigame message if the challenge is invalid.
    if (data == null) {
      return;
    }

    // Create alert view with Icon
    AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab_Icon, overrideCloseOnTouchOutside: false);
    // Hook up callbacks
    alertView.SetCloseButtonEnabled(false);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleMiniGameConfirm);
    alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, LearnToCopeWithMiniGameRejection);
    alertView.SetIcon(_LastChallengeData.ChallengeIcon);
    alertView.ViewClosed += HandleRequestDialogClose;
    alertView.TitleLocKey = LocalizationKeys.kRequestGameTitle;
    alertView.DescriptionLocKey = LocalizationKeys.kRequestGameDescription;
    alertView.SetTitleArgs(new object[] { Localization.Get(_LastChallengeData.ChallengeTitleLocKey) });
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedEnd);
    _RequestPending = false;
    _RequestDialog = alertView;
  }

  private void LearnToCopeWithMiniGameRejection() {
    DAS.Info(this, "LearnToCopeWithMiniGameRejection");
    RobotEngineManager.Instance.SendDenyGameStart();
  }

  private void HandleMiniGameConfirm() {
    DAS.Info(this, "HandleMiniGameConfirm");
    _RequestPending = true;
    if (_RequestDialog != null) {
      _RequestDialog.DisableAllButtons();
      _RequestDialog.ViewClosed -= HandleRequestDialogClose;
    }
    RobotEngineManager.Instance.CurrentRobot.SendAnimationGroup(AnimationGroupName.kRequestGame_Confirm, HandleMiniGameYesAnimEnd);
  }

  private void HandleMiniGameYesAnimEnd(bool success) {
    DAS.Info(this, "HandleMiniGameYesAnimEnd");
    MinigameConfirmed.Invoke(_LastChallengeData.ChallengeID);
  }

  private void HandleExternalRejection(Anki.Cozmo.ExternalInterface.DenyGameStart message) {
    DAS.Info(this, "HandleExternalRejection");
    if (_RequestDialog != null && _RequestPending == false) {
      _RequestDialog.CloseView();
    }
  }

  private void HandleRequestDialogClose() {
    DAS.Info(this, "HandleUnexpectedClose");
    if (_RequestDialog != null) {
      _RequestDialog.ViewClosed -= HandleRequestDialogClose;
      SetMinigameNeed();
    }
  }

  #region Calculate Friendship Points and DailyGoal Progress

  public bool AreAllDailyGoalsComplete() {
    return GetTodayProgress() >= 1.0f;
  }

  #endregion

}

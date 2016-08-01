using System;
using UnityEngine;
using DataPersistence;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;
using Newtonsoft.Json;
using System.IO;
using Anki.Cozmo;

/// <summary>
/// Daily goal manager. Loads in DailyGoal information and manages 
/// </summary>
public class DailyGoalManager : MonoBehaviour {

  // List of Current Generation Data
  private DailyGoalGenerationData _CurrentGenData;

  public static string sDailyGoalDirectory { get { return PlatformUtil.GetResourcesFolder("DailyGoals"); } }

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

  #region DailyGoal Generation

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

  public List<string> GetCurrentDailyGoalNames() {
    List<string> dailyGoalList = new List<string>();
    if (DataPersistenceManager.Instance.CurrentSession != null) {
      for (int i = 0; i < DataPersistenceManager.Instance.CurrentSession.DailyGoals.Count; i++) {
        dailyGoalList.Add(DataPersistenceManager.Instance.CurrentSession.DailyGoals[i].Title);
      }
    }
    return dailyGoalList;
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
        BehaviorGameFlag bGame = GetRequestBehaviorGameFlag(_LastChallengeData.ChallengeID);
        // Do not reate the minigame message if the behavior group is invalid.
        if (bGame == BehaviorGameFlag.NoGame) {
          return null;
        }
        DisableRequestGameBehaviorGroups();
        RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(bGame);
        return _LastChallengeData;
      }
    }
    DAS.Error(this, string.Format("Challenge ID not found in ChallengeList {0}", config.ChallengeID));
    return null;
  }

  public BehaviorGameFlag GetRequestBehaviorGameFlag(string challengeID) {
    for (int i = 0; i < _RequestMinigameConfig.RequestList.Length; i++) {
      if (challengeID == _RequestMinigameConfig.RequestList[i].ChallengeID) {
        return _RequestMinigameConfig.RequestList[i].RequestBehaviorGameFlag;
      }
    }
    DAS.Error(this, string.Format("Challenge ID not found in RequestMinigameList {0}", challengeID));
    return BehaviorGameFlag.NoGame;
  }

  public void DisableRequestGameBehaviorGroups() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.NoGame);
    }
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
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);
    _CurrentGenData = new DailyGoalGenerationData();
    // Load all Event Map Configs (Can have multiple, so you can create different configs, game only uses one.)
    if (Directory.Exists(sDailyGoalDirectory)) {
      string[] _DailyGoalFiles = Directory.GetFiles(sDailyGoalDirectory);
      if (_DailyGoalFiles.Length > 0) {
        bool didMatch = false;
        for (int i = 0; i < _DailyGoalFiles.Length; i++) {
          if (_DailyGoalFiles[i] == Path.Combine(sDailyGoalDirectory, _DailyGoalGenConfig.DailyGoalFileName)) {
            LoadDailyGoalData(_DailyGoalFiles[i]);
            didMatch = true;
            break;
          }
        }
        if (!didMatch) {
          DAS.Warn(this, string.Format("Could not find {0}, defaulting to {1}", _DailyGoalGenConfig.DailyGoalFileName, _DailyGoalFiles[0]));
          LoadDailyGoalData(_DailyGoalFiles[0]);
        }
      }
      else {
        DAS.Error(this, string.Format("No DailyGoal Data to load in {0}", sDailyGoalDirectory));
      }
    }
    else {
      DAS.Error(this, string.Format("No DailyGoal Data to load in {0}", sDailyGoalDirectory));
    }
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);
  }

  private void LoadDailyGoalData(string path) {
    string json = File.ReadAllText(path);
    DAS.Event(this, string.Format("LoadDailyGoalData from {0}", Path.GetFileName(path)));
    _CurrentGenData = JsonConvert.DeserializeObject<DailyGoalGenerationData>(json, GlobalSerializerSettings.JsonSettings);
  }

  public List<DailyGoal> GenerateDailyGoals() {
    List<DailyGoal> newGoals = new List<DailyGoal>();
    int goalCount = Mathf.Min(_CurrentGenData.GenList.Count, UnityEngine.Random.Range(_DailyGoalGenConfig.MinGoals, _DailyGoalGenConfig.MaxGoals + 1));
    List<DailyGoalGenerationData.GoalEntry> goalList = new List<DailyGoalGenerationData.GoalEntry>();
    // Look at a list of exclusively goals that have their conditions met
    for (int i = 0; i < _CurrentGenData.GenList.Count; i++) {
      if (_CurrentGenData.GenList[i].CanGen()) {
        goalList.Add(_CurrentGenData.GenList[i]);
      }
    }
    goalCount = Mathf.Min(goalCount, goalList.Count);
    // Grab random DailyGoals from the available goal list
    DailyGoalGenerationData.GoalEntry toAdd;
    for (int i = 0; i < goalCount; i++) {
      toAdd = goalList[UnityEngine.Random.Range(0, goalList.Count)];
      // Remove from list to prevent dupes
      goalList.Remove(toAdd);
      newGoals.Add(new DailyGoal(toAdd.CladEvent, toAdd.TitleKey, toAdd.PointsRewarded, toAdd.Target, toAdd.RewardType, toAdd.ProgressConditions, toAdd.Priority));
    }
    SendDasEventsForGoalGeneration(newGoals);
    return newGoals;
  }

  private void SendDasEventsForGoalGeneration(List<DailyGoal> goals) {
    if (goals.Count > 0) {
      for (int i = 0; i < goals.Count; i++) {
        DAS.Event(DASConstants.Goal.kGeneration, DASUtil.FormatDate(DataPersistenceManager.Today),
          new Dictionary<string, string> { {
              "$data",
              DASUtil.FormatGoal(goals[i])
            }
          });
      }
    }
  }

  private void HandleAskForMinigame(object messageObject) {
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
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Request_Game);
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
    HandleMiniGameYesAnimEnd(true);
  }

  private void HandleMiniGameYesAnimEnd(bool success) {
    DAS.Info(this, "HandleMiniGameYesAnimEnd");
    MinigameConfirmed.Invoke(_LastChallengeData.ChallengeID);
  }

  private void HandleExternalRejection(object messageObject) {
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

  public int GetConfigMaxGoalCount() {
    return _DailyGoalGenConfig.MaxGoals;
  }
}

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

  public Action OnRefreshDailyGoals;

  // List of Current Generation Data
  private DailyGoalGenerationData _CurrentGenData;

  public static string sDailyGoalDirectory { get { return PlatformUtil.GetResourcesFolder("DailyGoals"); } }

  #region constants

  private const float _kMinigameNeedMin = 0.3f;
  private const float _kMinigameNeedMax = 1.0f;

  #endregion

  [SerializeField]
  private ChallengeDataList _ChallengeList;

  public ChallengeData CurrentChallengeToRequest {
    get {
      return _LastChallengeData;
    }
  }
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

  public List<DailyGoalGenerationData.GoalEntry> GetGeneratableGoalEntries() {
    List<DailyGoalGenerationData.GoalEntry> goalList = new List<DailyGoalGenerationData.GoalEntry>();
    // Look at a list of exclusively goals that have their conditions met
    for (int i = 0; i < _CurrentGenData.GenList.Count; i++) {
      if (_CurrentGenData.GenList[i].CanGen()) {
        goalList.Add(_CurrentGenData.GenList[i]);
      }
    }
    return goalList;
  }

  public List<string> GetGeneratableGoalNames() {
    List<string> goalNameList = new List<string>();
    List<DailyGoalGenerationData.GoalEntry> goalList = new List<DailyGoalGenerationData.GoalEntry>();
    goalList = GetGeneratableGoalEntries();
    for (int i = 0; i < goalList.Count; i++) {
      goalNameList.Add(Localization.Get(goalList[i].TitleKey));
    }
    return goalNameList;
  }

  [SerializeField]
  private RequestGameListConfig _RequestMinigameConfig;

  public RequestGameListConfig GetRequestMinigameConfig() {
    return _RequestMinigameConfig;
  }

  public ChallengeData PickMiniGameToRequest() {
    List<RequestGameConfig> unlockedList = new List<RequestGameConfig>();

    for (int i = 0; i < _RequestMinigameConfig.RequestList.Length; ++i) {
      UnlockId unlockid = UnlockId.Count;
      for (int j = 0; j < _ChallengeList.ChallengeData.Length; ++j) {
        if (_ChallengeList.ChallengeData[j].ChallengeID == _RequestMinigameConfig.RequestList[i].ChallengeID) {
          unlockid = _ChallengeList.ChallengeData[j].UnlockId.Value;
          break;
        }
      }
      if (UnlockablesManager.Instance.IsUnlocked(unlockid)) {
        unlockedList.Add(_RequestMinigameConfig.RequestList[i]);
      }
    }
    if (unlockedList.Count == 0) {
      return null;
    }

    RequestGameConfig config = unlockedList[UnityEngine.Random.Range(0, unlockedList.Count)];

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
    ChallengeData data = PickMiniGameToRequest();
    // no minigames are unlocked...
    if (data == null) {
      prog = 0;
    }
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
    _CurrentGenData = new DailyGoalGenerationData();
    // Load all Event Map Configs (Can have multiple, so you can create different configs, game only uses one.)
    if (Directory.Exists(sDailyGoalDirectory)) {
      string[] _DailyGoalFiles = Directory.GetFiles(sDailyGoalDirectory);
      if (_DailyGoalFiles.Length > 0) {
        string dailyGoalsFileName = GetDailyGoalFileName();
        bool didMatch = false;
        for (int i = 0; i < _DailyGoalFiles.Length; i++) {
          if (_DailyGoalFiles[i] == Path.Combine(sDailyGoalDirectory, dailyGoalsFileName)) {
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

  private string GetDailyGoalFileName() {
    if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.DailyGoals)) {
      return _DailyGoalGenConfig.OnboardingGoalFileName;
    }
    return _DailyGoalGenConfig.DailyGoalFileName;
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
    goalList = GetGeneratableGoalEntries();
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
    if (OnRefreshDailyGoals != null) {
      OnRefreshDailyGoals.Invoke();
    }
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

  #region Calculate Friendship Points and DailyGoal Progress

  public bool AreAllDailyGoalsComplete() {
    return GetTodayProgress() >= 1.0f;
  }

  #endregion

  public int GetConfigMaxGoalCount() {
    return _DailyGoalGenConfig.MaxGoals;
  }
}

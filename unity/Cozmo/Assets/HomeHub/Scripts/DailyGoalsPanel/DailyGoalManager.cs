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

  #region Pending Goal Logic for HomeView animations

  // DailyGoals that have been earned but haven't been shown to the player.
  public List<DailyGoal> PendingDailyGoals = new List<DailyGoal>();

  public bool GoalsPending {
    get {
      return PendingDailyGoals.Count > 0;
    }
  }

  private DailyGoalPanel _GoalPanelInstance = null;
  public DailyGoalPanel GoalPanelInstance {
    get {
      return _GoalPanelInstance;
    }
    set {
      _GoalPanelInstance = value;
    }
  }

  #endregion

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
      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.DailyGoals) ||
                                                          _CurrentGenData.GenList[i].CanGen()) {
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
    LoadDailyGoalGenerationData();
    GameEventManager.Instance.OnGameEvent += HandleGameEvent;
  }

  void OnDestroy() {
    ResolveDailyGoalsEarned();
    GameEventManager.Instance.OnGameEvent -= HandleGameEvent;
  }

  public void LoadDailyGoalGenerationData() {
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
    DAS.Debug(this, string.Format("LoadDailyGoalData from {0}", Path.GetFileName(path)));
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
      // Clear out tag collisions
      if (TagConfig.IsValidTag(toAdd.Tag)) {
        for (int j = 0; j < goalList.Count;) {
          if (goalList[j].Tag == toAdd.Tag) {
            goalList.RemoveAt(j);
          }
          else {
            j++;
          }
        }
      }
      DailyGoal newGoal = new DailyGoal(toAdd.CladEvent, toAdd.TitleKey, toAdd.PointsRewarded, toAdd.Target, toAdd.RewardType, toAdd.ProgressConditions, toAdd.GenConditions, toAdd.Priority);
      newGoals.Add(newGoal);
      // If all remaining generatable goals have been removed due to tag conflicts, then don't bother generating any more, even if we have fewer
      // than the min number of goals for the day.
      if (goalList.Count <= 0) {
        break;
      }
    }
    // Sort by priority, placing higher priority at the front of the list
    newGoals.Sort((Cozmo.UI.DailyGoal x, Cozmo.UI.DailyGoal y) => {
      return y.Priority.CompareTo(x.Priority);
    });
    SendDasEventsForGoalGeneration(newGoals);
    return newGoals;
  }


  private void HandleGameEvent(GameEventWrapper gameEvent) {
    if (gameEvent.GameEventEnum == Anki.Cozmo.GameEvent.OnDailyGoalCompleted) {
      DailyGoalCompleteGameEvent goalEvent = gameEvent as DailyGoalCompleteGameEvent;

      if (PendingDailyGoals == null) {
        DAS.Error("DailyGoalManager.HandleGameEvent.NullCheck", "PendingDailyGoals null");
      }

      // Only add to Pending Goals if it is unique, fire Warning otherwise since
      // that suggests dupe goals or some other funny business I certainly don't
      // approve of.
      if (!PendingDailyGoals.Contains(goalEvent.CompletedGoal)) {
        PendingDailyGoals.Add(goalEvent.CompletedGoal);
      }
      else {
        DAS.Warn("DailyGoalManager.HandleDailyGoalCompleted", "Duplicate DailyGoal completed event fired");
      }
    }
    else {
      if (DataPersistenceManager.Instance == null) {
        DAS.Error("DailyGoalManager.HandleGameEvent.NullCheck", "Data persistence manager is null");
      }

      if (DataPersistenceManager.Instance.CurrentSession == null) {
        DAS.Error("DailyGoalManager.HandleGameEvent.NullCheck", "CurrentSession is null");
      }

      List<DailyGoal> goals = DataPersistenceManager.Instance.CurrentSession.DailyGoals;

      if (goals != null) {
        for (int i = 0; i < goals.Count; i++) {
          GoalProgCheck(goals[i], gameEvent);
        }
      }
      else {
        DAS.Error("DailyGoalManager.HandleGameEvent.NullCheck", "CurrentSession.DailyGoals is null");
      }

    }
  }

  public void GoalProgCheck(DailyGoal goal, GameEventWrapper gEvent) {
    if (gEvent.GameEventEnum != goal.GoalEvent) {
      return;
    }
    // If ProgConditions aren't met, don't progress
    if (!goal.CanProgress(gEvent)) {
      return;
    }
    // Progress Goal
    goal.Progress++;
    GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnDailyGoalProgress, this));

    DAS.Event("meta.goal.progressed", goal.Title.Key, DASUtil.FormatExtraData(goal.Progress.ToString()));
    // Check if Completed
    goal.CheckIfComplete();
    if (goal.OnDailyGoalUpdated != null) {
      goal.OnDailyGoalUpdated.Invoke(goal);
    }
  }


  public void ResolveDailyGoalsEarned() {
    if (PendingDailyGoals == null) {
      return;
    }
    for (int i = 0; i < PendingDailyGoals.Count; i++) {
      string rewardType = PendingDailyGoals[i].RewardType;
      int pointsRewarded = PendingDailyGoals[i].PointsRewarded;
      DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount(rewardType, pointsRewarded);
    }
    DataPersistenceManager.Instance.Save();
    PendingDailyGoals.Clear();

  }

  private void SendDasEventsForGoalGeneration(List<DailyGoal> goals) {
    if (goals.Count > 0) {
      for (int i = 0; i < goals.Count; i++) {
        DAS.Event(DASConstants.Goal.kGeneration, DASUtil.FormatDate(DataPersistenceManager.Today),
          DASUtil.FormatExtraData(DASUtil.FormatGoal(goals[i])));
      }
    }
  }

  public bool AreAllDailyGoalsComplete() {
    return GetTodayProgress() >= 1.0f;
  }

  /// <summary>
  /// Check current session's goals, if any of them could not be generated with
  /// current game state because we have already unlocked the unlockID/Difficulty level
  /// then assume we've already completed the goal and auto complete them.
  /// </summary>
  public void ValidateExistingGoals() {
    if (DataPersistenceManager.Instance.CurrentSession != null) {
      for (int i = 0; i < DataPersistenceManager.Instance.CurrentSession.DailyGoals.Count; i++) {
        DailyGoal toCheck = DataPersistenceManager.Instance.CurrentSession.DailyGoals[i];
        if (!toCheck.GoalComplete) {
          toCheck.CheckAndResolveInvalidGenConditions();
        }
      }
    }
  }

  public int GetConfigMaxGoalCount() {
    return _DailyGoalGenConfig.MaxGoals;
  }
}

using System;
using UnityEngine;
using DataPersistence;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;

public class DailyGoalManager : MonoBehaviour {

  public Action<string> MinigameConfirmed;

  [SerializeField]
  private ChallengeDataList _ChallengeList;

  private AlertView _RequestDialog = null;

  // The Last Challenge ID Cozmo has requested to play
  private string _lastChallengeID;

  #region FriendshipProgression and DailyGoals

  // Config file for friendship progression and daily goal generation
  [SerializeField]
  private FriendshipProgressionConfig _FriendshipProgConfig;

  public FriendshipProgressionConfig GetFriendshipProgressConfig() {
    return _FriendshipProgConfig;
  }

  [SerializeField]
  private FriendshipFormulaConfiguration _FriendshipFormulaConfig;

  public FriendshipFormulaConfiguration GetFriendForumulaConfig() {
    return _FriendshipFormulaConfig;
  }

  public bool HasGoalForStat(Anki.Cozmo.ProgressionStatType type) {
    int val = 0;
    if (DataPersistenceManager.Instance.CurrentSession.Goals.TryGetValue(type, out val)) {
      if (val > 0) {
        return true;
      }
    }
    return false;
  }

  public float GetDailyProgress() {
    StatContainer prog = DataPersistenceManager.Instance.CurrentSession.Progress;
    StatContainer goal = DataPersistenceManager.Instance.CurrentSession.Goals;
    return _FriendshipFormulaConfig.CalculateDailyGoalProgress(prog, goal);
  }

  public string GetDesiredMinigameID() {
    ChallengeData randC = _ChallengeList.ChallengeData[UnityEngine.Random.Range(0, _ChallengeList.ChallengeData.Length)];
    _lastChallengeID = randC.ChallengeID;
    return _lastChallengeID;
  }

  /// <summary>
  /// Returns the current goal that's the furthest from being complete.
  /// Use to help determine which minigame cozmo wants to play
  /// </summary>
  /// <returns>The stat.</returns>
  public Anki.Cozmo.ProgressionStatType PrimaryStat() {
    int greatestVal = 0;
    Anki.Cozmo.ProgressionStatType greatestType = Anki.Cozmo.ProgressionStatType.Count;
    foreach (KeyValuePair<Anki.Cozmo.ProgressionStatType, int> kVp in DataPersistenceManager.Instance.CurrentSession.Goals) {
      int currProg = 0;
      DataPersistenceManager.Instance.CurrentSession.Progress.TryGetValue(kVp.Key, out currProg);
      int currGoal = kVp.Value - currProg;
      if (currGoal > greatestVal) {
        greatestVal = currGoal;
        greatestType = kVp.Key;
      }
    }
    return greatestType;
  }

  /// <summary>
  ///  Returns a value between -1 and 1 based on how close AND far you are from completing daily goal
  /// </summary>
  /// <returns>The minigame need.</returns>
  public float GetMinigameNeed_Extremes() {
    float prog = (GetDailyProgress() * 2.0f) - 1;
    // Calculate how far you are from 50% complete
    // range from 0 -> 1.0
    prog = (Mathf.Abs(prog) * 2.0f) - 1;
    return prog;
  }

  /// <summary>
  ///  Returns a value between -1 and 1 based on how close you are to completing daily goal
  /// </summary>
  /// <returns>The minigame need.</returns>
  public float GetMinigameNeed_Close() {
    float prog = GetDailyProgress();
    prog = (prog - 0.5f) * 2.0f;
    return prog;
  }

  /// <summary>
  ///  Returns a value between -1 and 1 based on how far you are to completing daily goal
  /// </summary>
  /// <returns>The minigame need.</returns>
  public float GetMinigameNeed_Far() {
    float prog = GetDailyProgress();
    prog = (0.5f - prog) * 2.0f;
    return prog;
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

  void Awake() {
    Instance = this;
    RobotEngineManager.Instance.OnRequestGameStart += HandleAskForMinigame;
    RobotEngineManager.Instance.OnDenyGameStart += HandleExternalRejection;
  }
	
  // Using current friendship level and the appropriate config file,
  // generate a series of random goals for the day.
  public StatContainer GenerateDailyGoals() {
    Robot rob = RobotEngineManager.Instance.CurrentRobot;
    FriendshipProgressionConfig config = _FriendshipProgConfig;

    int lvl = rob.FriendshipLevel;
    if (lvl >= config.FriendshipLevels.Length) {
      lvl = config.FriendshipLevels.Length - 1;
    }
    DAS.Event(this, string.Format("GoalGeneration({0},{1})", lvl, rob.GetFriendshipLevelName(lvl)));
    StatBitMask possibleStats = StatBitMask.None;
    int totalGoals = 0;
    int min = 0;
    int max = 0;
    // Iterate through each level and add in the stats introduced for that level
    for (int i = 0; i <= lvl; i++) {
      possibleStats |= config.FriendshipLevels[i].StatsIntroduced;
    }
    totalGoals = config.FriendshipLevels[lvl].MaxGoals;
    min = config.FriendshipLevels[lvl].MinTarget;
    max = config.FriendshipLevels[lvl].MaxTarget;

    // Don't generate more goals than possible stats
    if (totalGoals > possibleStats.Count) {
      DAS.Warn(this, "More Goals than Potential Stats");
      totalGoals = possibleStats.Count;
    }
    StatContainer goals = new StatContainer();
    // Generate Goals from the possible stats, set stats to false as you pick them
    // to prevent duplicates.
    for (int i = 0; i < totalGoals; i++) {
      Anki.Cozmo.ProgressionStatType targetStat = possibleStats.Random();
      possibleStats[targetStat] = false;
      goals[targetStat] = UnityEngine.Random.Range(min, max);
    }
    return goals;
  }

  private void HandleAskForMinigame(Anki.Cozmo.ExternalInterface.RequestGameStart message) {
    if (_RequestDialog != null) {
      // Avoid dupes
      return;
    }
    GetDesiredMinigameID();
    // TODO: When the message has the appropriate 
    AlertView alertView = UIManager.OpenView(UIPrefabHolder.Instance.AlertViewPrefab) as AlertView;
    // Hook up callbacks
    alertView.SetCloseButtonEnabled(false);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleMiniGameConfirm);
    alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, LearnToCopeWithMiniGameRejection);
    alertView.TitleLocKey = LocalizationKeys.kRequestGameTitle;
    alertView.DescriptionLocKey = LocalizationKeys.kRequestGameDescription;
    alertView.SetMessageArgs(new object[] { _lastChallengeID });
    _RequestDialog = alertView;
  }

  private void LearnToCopeWithMiniGameRejection() {
    DAS.Info(this, "LearnToCopeWithMiniGameRejection");
    if (_RequestDialog != null) {
      _RequestDialog.CloseView();
    }
    RobotEngineManager.Instance.SendDenyGameStart();
  }

  private void HandleMiniGameConfirm() {
    DAS.Info(this, "HandleMiniGameConfirm");
    MinigameConfirmed.Invoke(_lastChallengeID);
  }

  private void HandleExternalRejection(Anki.Cozmo.ExternalInterface.DenyGameStart message) {
    DAS.Info(this, "HandleExternalRejection"); 
    if (_RequestDialog != null) {
      _RequestDialog.CloseView();
    }
  }

}

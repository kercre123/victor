using System;
using UnityEngine;
using DataPersistence;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;
using Anki.Cozmo;

public class DailyGoalManager : MonoBehaviour {

  #region constants

  private const float _kMinigameNeedMin = 0.3f;
  private const float _kMinigameNeedMax = 1.0f;

  #endregion

  public Action<string> MinigameConfirmed;

  [SerializeField]
  private ChallengeDataList _ChallengeList;

  private AlertView _RequestDialog = null;

  // The Last Challenge ID Cozmo has requested to play
  private ChallengeData _LastChallengeData;

  #region FriendshipProgression and DailyGoals

  // Config file for friendship progression and daily goal generation
  [SerializeField]
  private FriendshipProgressionConfig _FriendshipProgConfig;

  public FriendshipProgressionConfig GetFriendshipProgressConfig() {
    return _FriendshipProgConfig;
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

  public float GetTodayProgress() {
    if (DataPersistenceManager.Instance.CurrentSession != null) {
      StatContainer prog = DataPersistenceManager.Instance.CurrentSession.Progress;
      StatContainer goal = DataPersistenceManager.Instance.CurrentSession.Goals;
      return CalculateDailyGoalProgress(prog, goal);
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
    Debug.LogError(string.Format("Challenge ID not found in ChallengeList {0}", config.ChallengeID));
    return null;
  }

  public BehaviorGroup GetRequestBehaviorGroup(string challengeID) {
    for (int i = 0; i < _RequestMinigameConfig.RequestList.Length; i++) {
      if (challengeID == _RequestMinigameConfig.RequestList[i].ChallengeID) {
        return _RequestMinigameConfig.RequestList[i].RequestBehaviorGroup;
      }
    }
    Debug.LogError(string.Format("Challenge ID not found in RequestMinigameList {0}", challengeID));
    return BehaviorGroup.MiniGame;
  }

  public void DisableRequestGameBehaviorGroups() {
    RobotEngineManager.Instance.CurrentRobot.SetEnableBehaviorGroup(BehaviorGroup.MiniGame, false);
    RobotEngineManager.Instance.CurrentRobot.SetEnableBehaviorGroup(BehaviorGroup.RequestSpeedTap, false);
    RobotEngineManager.Instance.CurrentRobot.SetEnableBehaviorGroup(BehaviorGroup.RequestCubePounce, false);
    RobotEngineManager.Instance.CurrentRobot.SetEnableBehaviorGroup(BehaviorGroup.RequestSimon, false);
  }

  /// <summary>
  /// Returns the current goal that's the furthest from being complete.
  /// Use to help determine which minigame cozmo wants to play
  /// NOTE - Currently not in use
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
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.OnRequestGameStart -= HandleAskForMinigame;
    RobotEngineManager.Instance.OnDenyGameStart -= HandleExternalRejection;
  }
	
  // Using current friendship level and the appropriate config file,
  // generate a series of random goals for the day.
  public StatContainer GenerateDailyGoals() {

    IRobot rob = RobotEngineManager.Instance.CurrentRobot;
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

    SendDasEventsForGoalGeneration(goals);

    return goals;
  }

  private void SendDasEventsForGoalGeneration(StatContainer goals) {
    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; i++) {
      Anki.Cozmo.ProgressionStatType index = (Anki.Cozmo.ProgressionStatType)i;
      if (goals[index] > 0) {
        DAS.Event(DASConstants.Goal.kGeneration, DASUtil.FormatDate(DataPersistenceManager.Today),
          new Dictionary<string,string> {
            { "$data", DASUtil.FormatStatAmount(index, goals[index]) }
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
    AlertView alertView = UIManager.OpenView(UIPrefabHolder.Instance.AlertViewPrefab_Icon, overrideCloseOnTouchOutside: false);
    // Hook up callbacks
    alertView.SetCloseButtonEnabled(false);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleMiniGameConfirm);
    alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, LearnToCopeWithMiniGameRejection);
    alertView.SetIcon(data.ChallengeIcon);
    alertView.ViewClosed += HandleRequestDialogClose;
    alertView.TitleLocKey = LocalizationKeys.kRequestGameTitle;
    alertView.DescriptionLocKey = LocalizationKeys.kRequestGameDescription;
    alertView.SetTitleArgs(new object[] { Localization.Get(data.ChallengeTitleLocKey) });
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
    MinigameConfirmed.Invoke(_LastChallengeData.ChallengeID);
  }

  private void HandleExternalRejection(Anki.Cozmo.ExternalInterface.DenyGameStart message) {
    DAS.Info(this, "HandleExternalRejection");
    if (_RequestDialog != null) {
      _RequestDialog.CloseView();
    }
  }

  private void HandleRequestDialogClose() {
    DAS.Info(this, "HandleUnexpectedClose");
    SetMinigameNeed();
    if (_RequestDialog != null) {
      _RequestDialog.ViewClosed -= HandleRequestDialogClose;
    }
  }

  #region Calculate Friendship Points and DailyGoal Progress

  // Returns the % progression towards your daily goals
  // Range from 0-1. Does not take overflow into account.
  public float CalculateDailyGoalProgress(StatContainer progress, StatContainer goal) {
    int totalProgress = 0, totalGoal = 0;
    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; i++) {
      var stat = (Anki.Cozmo.ProgressionStatType)i;
      totalProgress += Mathf.Min(progress[stat], goal[stat]);
      totalGoal += goal[stat];
    }
    return ((float)totalProgress / (float)totalGoal);
  }

  // Calculates friendship points earned for the day based on stats earned and Bonus Mult
  public int CalculateFriendshipPoints(StatContainer progress, StatContainer goal) {
    int totalProgress = 0;
    float mult = CalculateBonusMult(progress, goal);
    if (mult < 1.0f) {
      mult = 1.0f;
    }
    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; i++) {
      var stat = (Anki.Cozmo.ProgressionStatType)i;
      totalProgress += progress[stat];
    }
    return (totalProgress * Mathf.CeilToInt(mult));
  }

  // Calculates the current Bonus Multiplier for calculating friendship points
  public float CalculateBonusMult(StatContainer progress, StatContainer goal) {
    int totalProgress = 0, totalGoal = 0;
    float bonusMult = 0.0f;
    if (AreAllDailyGoalsComplete(progress, goal) == true) {
      for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; i++) {
        var stat = (Anki.Cozmo.ProgressionStatType)i;
        totalProgress += progress[stat];
        totalGoal += goal[stat];
      }
      bonusMult = ((float)totalProgress / (float)totalGoal);
      // Register x2 mult including at exactly 100%
      if (bonusMult == 1.0f) {
        bonusMult += 0.001f;
      }
    }

    return bonusMult;
  }

  public bool AreAllDailyGoalsComplete(StatContainer progress, StatContainer goal) {
    bool isDone = true;
    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; i++) {
      var stat = (Anki.Cozmo.ProgressionStatType)i;
      if (progress[stat] < goal[stat]) {
        isDone = false;
        break;
      }
    }
    return isDone;
  }

  #endregion

}

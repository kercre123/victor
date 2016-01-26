using UnityEngine;
using DataPersistence;
using System.Collections;

public class DailyGoalManager : MonoBehaviour {
  
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

  /// <summary>
  ///  Returns a value between -1 and 1 based on how close AND far you are from completing daily goal
  /// </summary>
  /// <returns>The minigame need.</returns>
  public float GetMinigameNeed_Extremes() {
    float prog = GetDailyProgress();
    // Calculate how far you are from 50% complete
    // range from 0 -> 1.0
    prog = Mathf.Abs(prog - 0.5f) * 2.0f;
    // Multiply scale back out to -1 -> 1
    prog = (prog - 0.5f) * 2.0f;
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


  private DailyGoalPanel _GoalPanelRef;

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
      goals[targetStat] = Random.Range(min, max);
    }
    return goals;
  }


}

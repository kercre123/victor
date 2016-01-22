using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;

// Panel for generating and displaying the ProgressionStat goals for the Day.
public class DailyGoalPanel : BaseView {
  
  private readonly List<GoalCell> _GoalCells = new List<GoalCell>();

  // Force a specific Friendship level for Goal Generation testing,
  // must also set _UseDebug to true.
  [SerializeField]
  private int _DebugLevel = -1;
  [SerializeField]
  private bool _UseDebug = false;

  // Prefab for GoalCells
  [SerializeField]
  private GoalCell _GoalCellPrefab;

  // Progress bar for tracking total progress for all Goals
  [SerializeField]
  private ProgressBar _TotalProgressBar;

  // Container for Daily Goal Cells to be children of
  [SerializeField]
  private Transform _GoalContainer;

  // Config file for friendship progression
  private FriendshipProgressionConfig _Config;
  [SerializeField]
  private FriendshipFormulaConfiguration _FriendshipFormulaConfig;


  void Awake() {
    _Config = RobotEngineManager.Instance.GetFriendshipProgressConfig();
  }

  // Using current friendship level and the appropriate config file,
  // generate a series of random goals for the day.
  public StatContainer GenerateDailyGoals() {
    Robot rob = RobotEngineManager.Instance.CurrentRobot;

    string name = rob.GetFriendshipLevelName(rob.FriendshipLevel);
    if (_UseDebug && _DebugLevel != -1) {
      name = rob.GetFriendshipLevelName(_DebugLevel);
    }
    DAS.Event(this, string.Format("GoalGeneration({0})", name));
    StatBitMask possibleStats = StatBitMask.None;
    int totalGoals = 0;
    int min = 0;
    int max = 0;
    for (int i = 0; i < _Config.FriendshipLevels.Length; i++) {
      possibleStats |= _Config.FriendshipLevels[i].StatsIntroduced;
      if (_Config.FriendshipLevels[i].FriendshipLevelName == name) {
        totalGoals = _Config.FriendshipLevels[i].MaxGoals;
        min = _Config.FriendshipLevels[i].MinTarget;
        max = _Config.FriendshipLevels[i].MaxTarget;
        break;
      }
    }
    // Don't generate more goals than possible stats
    if (totalGoals > possibleStats.Count) {
      DAS.Warn(this, "More Goals than Potential Stats");
      totalGoals = possibleStats.Count;
    }
    StatContainer goals = new StatContainer();
    // Generate Goals from the possible stats
    for (int i = 0; i < totalGoals; i++) {
      Anki.Cozmo.ProgressionStatType targetStat = possibleStats.Random();
      possibleStats[targetStat] = false;
      goals[targetStat] = Random.Range(min, max);
      CreateGoalCell(targetStat, goals[targetStat], 0);
    }

    _TotalProgressBar.SetProgress(0f);
    return goals;
  }

  public void SetDailyGoals(StatContainer goals, StatContainer progress) {
    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; i++) {
      var targetStat = (Anki.Cozmo.ProgressionStatType)i;
      if (goals[targetStat] > 0) {
        CreateGoalCell(targetStat, goals[targetStat], progress[targetStat]);
      }
    }
    _TotalProgressBar.SetProgress(_FriendshipFormulaConfig.CalculateFriendshipProgress(progress, goals));
  }

  // Creates a goal badge based on a progression stat and adds to the DailyGoal in RobotEngineManager
  // Currently this will be additive so if multiple Goals are created with the same required type, they will be combined
  public GoalCell CreateGoalCell(Anki.Cozmo.ProgressionStatType type, int target, int goal) {
    DAS.Event(this, string.Format("CreateGoalBadge({0},{1})", type, target));
    GoalCell newBadge = UIManager.CreateUIElement(_GoalCellPrefab.gameObject, _GoalContainer).GetComponent<GoalCell>();
    RobotEngineManager.Instance.DailyGoals[(int)type] += target;
    string newName = type.ToString();
    newBadge.Initialize(newName, target, goal, type);
    _GoalCells.Add(newBadge);
    newBadge.OnProgChanged += UpdateTotalProgress;
    return newBadge;
  }


  // Listens for any goal Badge values you listen to changing.
  // On Change, updates the total progress achieved by all goalbadges.
  public void UpdateTotalProgress() {
    float total = _GoalCells.Count;
    float curr = 0.0f;
    for (int i = 0; i < _GoalCells.Count; i++) {
      curr += _GoalCells[i].Progress;
    }
    _TotalProgressBar.SetProgress(curr/total);
  }

  protected override void CleanUp() {
    for (int i = 0; i < _GoalCells.Count; i++) {
      _GoalCells[i].OnProgChanged -= UpdateTotalProgress;
    }
    _GoalCells.Clear();
  }

}

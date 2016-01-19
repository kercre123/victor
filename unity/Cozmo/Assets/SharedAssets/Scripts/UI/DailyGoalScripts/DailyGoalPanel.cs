using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;

public class DailyGoalPanel : BaseView {

  private readonly int[] _DailyGoals = new int[(int)Anki.Cozmo.ProgressionStatType.Count];
  private readonly List<GoalBadge> _GoalUIBadges = new List<GoalBadge>();
  [SerializeField]
  private string _DebugLevel = "";
  [SerializeField]
  private bool _UseDebug = false;
  [SerializeField]
  private GoalBadge _GoalBadgePrefab;
  [SerializeField]
  private ProgressBar _TotalProgressBar;
  [SerializeField]
  private readonly float _ExpandWidth = 800f;
  [SerializeField]
  private readonly float _CollapseWidth = 400f;
  [SerializeField]
  private Transform _GoalContainer;
  [SerializeField]
  private DailyGoalConfig _Config;

  private bool _Expanded = true;
  public bool Expand {
    get {
      return _Expanded;
    }
    set {
      // Expand between half and full size
      if (_Expanded != value) {
        _Expanded = value;
        for (int i = 0; i < _GoalUIBadges.Count; i++) {
          _GoalUIBadges[i].Expand(value);
        }
        RectTransform trans = GetComponent<RectTransform>();
        // TODO: DOScaleX does this wrong. Identify a different way to animate the scaling for this element.
        if (_Expanded) {
          trans.sizeDelta = new Vector2(_ExpandWidth, 0.0f);
        }
        else {
          trans.sizeDelta = new Vector2(_CollapseWidth, 0.0f);
        }
      }
    }
  }

  public bool HasGoalForStat(Anki.Cozmo.ProgressionStatType type) {
    for (int i = 0; i < _DailyGoals.Length; i++) {
      if (_DailyGoals[i] > 0) {
        return true;
      }
    }
    return false;
  }

  // Using current friendship level and the appropriate config file,
  // generate a series of random goals for the day.
  public StatContainer GenerateDailyGoals() {
    Robot rob = RobotEngineManager.Instance.CurrentRobot;

    string name = rob.GetFriendshipLevelName(rob.FriendshipLevel);
    if (_UseDebug && _DebugLevel != "") {
      name = _DebugLevel;
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
      CreateGoalBadge(targetStat, goals[targetStat], 0);
    }
    return goals;
  }

  public void SetDailyGoals(StatContainer goals, StatContainer progress) {
    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; i++) {
      var targetStat = (Anki.Cozmo.ProgressionStatType)i;
      if (goals[targetStat] > 0) {
        CreateGoalBadge(targetStat, goals[targetStat], progress[targetStat]);
      }
    }
  }


  // Creates a goal badge based on a progression stat
  public GoalBadge CreateGoalBadge(Anki.Cozmo.ProgressionStatType type, int target, int goal) {
    _DailyGoals[(int)type] += target;
    DAS.Event(this, string.Format("CreateGoalBadge({0},{1})", type, target));
    return CreateGoalBadge(type.ToString(), target, goal);
  }

  // Creates a goal badge based on an arbitrary string
  public GoalBadge CreateGoalBadge(string name, int target, int goal) {
    GoalBadge newBadge = UIManager.CreateUIElement(_GoalBadgePrefab.gameObject, _GoalContainer).GetComponent<GoalBadge>();
    newBadge.Initialize(name, target, goal);
    _GoalUIBadges.Add(newBadge);
    newBadge.Expand(_Expanded);
    newBadge.OnProgChanged += UpdateTotalProgress;
    return newBadge;
  }

  // Listens for any goal Badge values you listen to changing.
  // On Change, updates the total progress achieved by all goalbadges.
  public void UpdateTotalProgress() {
    float total = _GoalUIBadges.Count;
    float curr = 0.0f;
    for (int i = 0; i < _GoalUIBadges.Count; i++) {
      curr += _GoalUIBadges[i].Progress;
    }
    _TotalProgressBar.SetProgress(curr/total);
  }

  protected override void CleanUp() {
    for (int i = 0; i < _GoalUIBadges.Count; i++) {
      //TODO: Dismiss GoalBadges through themselves
      _GoalUIBadges[i].OnProgChanged -= UpdateTotalProgress;
      Destroy(_GoalUIBadges[i].gameObject);
    }
    _GoalUIBadges.Clear();
  }

}

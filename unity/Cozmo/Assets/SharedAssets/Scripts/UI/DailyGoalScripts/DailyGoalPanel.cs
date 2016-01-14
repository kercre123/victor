using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;

public class DailyGoalPanel : BaseView {

  private int[] DailyGoals = new int[(int)Anki.Cozmo.ProgressionStatType.Count];
  private List<GoalBadge> _GoalUIBadges;
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

	// Use this for initialization
	void Awake () {
    _GoalUIBadges = new List<GoalBadge>();
    for (int i = 0; i < DailyGoals.Length; i++) {
      DailyGoals[i] = 0;
    }
	}
	
	// Update is called once per frame
	void Update () {
	}

  public bool HasGoalForStat(Anki.Cozmo.ProgressionStatType type) {
    for (int i = 0; i < DailyGoals.Length; i++) {
      if (DailyGoals[i] > 0) {
        return true;
      }
    }
    return false;
  }

  // TODO: Using current friendship level and the appropriate config file,
  // generate a series of random goals for the day.
  public void GenerateDailyGoals() {
    Robot rob = RobotEngineManager.Instance.CurrentRobot;

    string name = rob.GetFriendshipLevelName(rob.FriendshipLevel);
    List<Anki.Cozmo.ProgressionStatType> PossibleStats = new List<Anki.Cozmo.ProgressionStatType>();
    int totalGoals = 0;
    for (int i = 0; i < _Config.FriendshipLevels.Length; i++) {
      for (int j = 0; j < _Config.FriendshipLevels[i].StatsIncluded.Length; j++) {
        if (!PossibleStats.Contains(_Config.FriendshipLevels[i].StatsIncluded[j])) {
          PossibleStats.Add(_Config.FriendshipLevels[i].StatsIncluded[j]);
        }
      }
      if (_Config.FriendshipLevels[i].FriendshipLevelName == name) {
        totalGoals = _Config.FriendshipLevels[i].MaxGoals;
        break;
      }
    }
    // Don't generate more goals than possible stats
    if (totalGoals > PossibleStats.Count) {
      Debug.LogWarning("More Goals that Potential Stats");
      totalGoals = PossibleStats.Count;
    }
    // Generate Goals from the possible stats
    for (int i = 0; i < totalGoals; i++) {
      Anki.Cozmo.ProgressionStatType targetStat = PossibleStats[Random.Range(0, PossibleStats.Count)];
      PossibleStats.Remove(targetStat);
      CreateGoalBadge(targetStat, Random.Range(_Config.MinDailyGoalTarget, _Config.MaxDailyGoalTarget));
    }
  }

  // Creates a goal badge based on a progression stat
  public GoalBadge CreateGoalBadge(Anki.Cozmo.ProgressionStatType type, int target) {
    DailyGoals[(int)type] += target;
    return CreateGoalBadge(type.ToString(), target);
  }

  // Creates a goal badge based on an arbitrary string
  public GoalBadge CreateGoalBadge(string name, int target) {
    GoalBadge newBadge = UIManager.CreateUIElement(_GoalBadgePrefab.gameObject, _GoalContainer).GetComponent<GoalBadge>();
    newBadge.Initialize(name,target,0);
    _GoalUIBadges.Add(newBadge);
    newBadge.Expand(_Expanded);
    newBadge.OnProgChanged += UpdateTotalProgress;
    return newBadge;
  }

  // Listens for any goal Badge values you listen to changing.
  // On Change, updates the total progress achieved by all goalbadges.
  public void UpdateTotalProgress() {
    float Total = _GoalUIBadges.Count;
    float Curr = 0.0f;
    for (int i = 0; i < _GoalUIBadges.Count; i++) {
      Curr += _GoalUIBadges[i].Progress;
    }
    _TotalProgressBar.SetProgress(Curr/Total);
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

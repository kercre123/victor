using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DataPersistence;
using Cozmo.UI;
using Anki.Cozmo;

public class DailySummaryPanel : BaseView {
  
  public DailyGoalPanel.OnFriendshipBarAnimateComplete FriendshipBarAnimateComplete;

  // yay magic numbers
  const float kInitialFillPoint = 0.275f;
  const float kFinalFillPoint = 0.835f;
  const float kFillDuration = 1.2f;

  [SerializeField]
  private Color _UnlockedLevelColor;

  [SerializeField]
  private Color _LockedLevelColor;

  [SerializeField]
  private AnkiTextLabel _Title;

  [SerializeField]
  private Image _FriendBar;

  [SerializeField]
  private ProgressBar _DailyProgressBar;

  [SerializeField]
  private RectTransform _ObjectivesContainer;

  [SerializeField]
  private RectTransform _ChallengesContainer;

  [SerializeField]
  private GameObject _ObjectivePrefab;

  [SerializeField]
  private GameObject _ChallengePrefab;

  [SerializeField]
  private AnkiTextLabel _CurrentFriendshipLevel;

  [SerializeField]
  private AnkiTextLabel _NextFriendshipLevel;

  [SerializeField]
  private Transform _BonusBarContainer;
  [SerializeField]
  private BonusBarPanel _BonusBarPrefab;
  private BonusBarPanel _BonusBarPanel;

  // Config file for friendship progression
  [SerializeField]
  private FriendshipProgressionConfig _Config;

  protected override void CleanUp() {

  }

  public void Initialize(TimelineEntryData data) {
    int day = data.Date.Day;
    int month = data.Date.Month;

    _Title.FormattingArgs = new object[] { month, day };
    _BonusBarPanel = UIManager.CreateUIElement(_BonusBarPrefab.gameObject, _BonusBarContainer).GetComponent<BonusBarPanel>();

    float dailyProg = DailyGoalManager.Instance.CalculateDailyGoalProgress(data.Progress, data.Goals);
    float bonusMult = DailyGoalManager.Instance.CalculateBonusMult(data.Progress, data.Goals);
    _DailyProgressBar.SetProgress(dailyProg);
    _BonusBarPanel.SetFriendshipBonus(bonusMult);

    for (int i = 0; i < (int)ProgressionStatType.Count; i++) {
      var stat = (ProgressionStatType)i;
      if (data.Goals[stat] > 0) {
        CreateGoalCell(stat, data.Progress[stat], data.Goals[stat]);
      }
    }

    //RectTransform subContainer = null;
    for (int i = 0; i < data.CompletedChallenges.Count; i++) {
      CreateChallengeBadge(data.CompletedChallenges[i].ChallengeId, _ChallengesContainer);
    }

  }

  // Creates a goal badge TODO: Kill this
  private GoalCell CreateGoalCell(ProgressionStatType type, int progress, int goal) {
    GoalCell newBadge = UIManager.CreateUIElement(_ObjectivePrefab.gameObject, _ObjectivesContainer).GetComponent<GoalCell>();
    newBadge.Initialize(type, progress, goal, false);
    return newBadge;
  }

  // Creates a goal badge TODO: Use this instead
  private GoalCell CreateGoalCell(DailyGoal goal) {
    GoalCell newBadge = UIManager.CreateUIElement(_ObjectivePrefab.gameObject, _ObjectivesContainer).GetComponent<GoalCell>();
    newBadge.Initialize(goal, false);
    return newBadge;
  }


  private ChallengeBadge CreateChallengeBadge(string challengeId, RectTransform container) {
    ChallengeBadge newBadge = UIManager.CreateUIElement(_ChallengePrefab.gameObject, container).GetComponent<ChallengeBadge>();
    newBadge.Initialize(challengeId);
    return newBadge;
  }

  // sets the labels on the friendship timeline
  private void SetFriendshipTexts(string current, string next, bool nextLocked = false) {
    _CurrentFriendshipLevel.text = current;
    _NextFriendshipLevel.text = next;
    if (nextLocked) {
      _NextFriendshipLevel.color = _LockedLevelColor;
    }
    else {
      _NextFriendshipLevel.color = _UnlockedLevelColor;
    }
  }
}

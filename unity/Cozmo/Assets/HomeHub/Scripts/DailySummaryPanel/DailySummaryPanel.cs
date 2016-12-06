using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DataPersistence;
using Cozmo.UI;
using Anki.Cozmo;

public class DailySummaryPanel : BaseModal {
  
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

  // Config file for friendship progression
  [SerializeField]
  private DailyGoalGenerationConfig _Config;

  protected override void CleanUp() {

  }

  public void Initialize(TimelineEntryData data) {
    int day = data.Date.Day;
    int month = data.Date.Month;

    _Title.FormattingArgs = new object[] { month, day };

    float dailyProg = data.GetTotalProgress();
    _DailyProgressBar.SetProgress(dailyProg);

    // Create a goal cell for each Daily Goal
    for (int i = 0; i < data.DailyGoals.Count; i++) {
      CreateGoalCell(data.DailyGoals[i]);
    }

    //RectTransform subContainer = null;
    for (int i = 0; i < data.CompletedChallenges.Count; i++) {
      CreateChallengeBadge(data.CompletedChallenges[i].ChallengeId, _ChallengesContainer);
    }

  }
    
  // Creates a goal badge
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

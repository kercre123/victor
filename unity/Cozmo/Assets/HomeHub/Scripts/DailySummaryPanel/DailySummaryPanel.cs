using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DataPersistence;
using Cozmo.UI;
using Anki.Cozmo;

public class DailySummaryPanel : BaseView {
  
  public Cozmo.HomeHub.TimelineView.OnFriendshipBarAnimateComplete FriendshipBarAnimateComplete;

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

    AnimateFriendshipBar(data);
  }

  public void AnimateFriendshipBar(TimelineEntryData data) {
    StartCoroutine(FriendshipBarCoroutine(data));
  }

  // Creates a goal badge
  private GoalCell CreateGoalCell(ProgressionStatType type, int progress, int goal) {
    GoalCell newBadge = UIManager.CreateUIElement(_ObjectivePrefab.gameObject, _ObjectivesContainer).GetComponent<GoalCell>();
    newBadge.Initialize(type, progress, goal, false);
    return newBadge;
  }

  private ChallengeBadge CreateChallengeBadge(string challengeId, RectTransform container) {
    ChallengeBadge newBadge = UIManager.CreateUIElement(_ChallengePrefab.gameObject, container).GetComponent<ChallengeBadge>();
    newBadge.Initialize(challengeId);
    return newBadge;
  }

  private IEnumerator FriendshipBarFill(float start, float end, float timeLimit) {
    start = Mathf.Lerp(kInitialFillPoint, kFinalFillPoint, start);
    end = Mathf.Lerp(kInitialFillPoint, kFinalFillPoint, end);

    var startTime = Time.time;

    while (true) {
      float progress = Mathf.Clamp01((Time.time - startTime) / timeLimit);

      float fill = Mathf.Lerp(start, end, progress);

      _FriendBar.fillAmount = fill;

      if (Time.time - startTime > timeLimit) {
        yield break;
      }
      yield return null;
    }
  }

  private IEnumerator FriendshipBarCoroutine(TimelineEntryData data) {

    var levelConfig = DailyGoalManager.Instance.GetFriendshipProgressConfig();

    int startingPoints = data.StartingFriendshipPoints;
    float pointsRequired, startingPercent, endingPercent;

    // first go through all levels that we reached
    for (int level = data.StartingFriendshipLevel; 
          level < data.EndingFriendshipLevel; level++) {

      // set up the labels
      SetFriendshipTexts(
        levelConfig.FriendshipLevels[level].FriendshipLevelName,
        levelConfig.FriendshipLevels[level + 1].FriendshipLevelName);

      pointsRequired = levelConfig.FriendshipLevels[level + 1].PointsRequired;

      startingPercent = startingPoints / pointsRequired;

      // fill the progress bar to the top
      yield return StartCoroutine(FriendshipBarFill(startingPercent, 1f, kFillDuration / (data.EndingFriendshipLevel - level)));

      // if there is another level after the next, play the animation
      // where the next dot slides to where the current dot is
      if (level + 2 < levelConfig.FriendshipLevels.Length) {
        yield return StartCoroutine(FriendshipBarFill(1, 0, 0.0f));
      }
      else {
        // otherwise, we are at max level. fill the next circle and keep it there
        yield break;
      }

      // our next fill will start at point 0
      startingPoints = 0;
    }

    // after filling all completed levels, we want to animate our progress on the current level
    int endingPoints = data.EndingFriendshipPoints;

    // if this isn't the max level, show our progress toward our next level
    if (data.EndingFriendshipLevel + 1 < levelConfig.FriendshipLevels.Length) {

      SetFriendshipTexts(
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel].FriendshipLevelName,
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel + 1].FriendshipLevelName, true);

      pointsRequired = levelConfig.FriendshipLevels[data.EndingFriendshipLevel + 1].PointsRequired;

      startingPercent = startingPoints / pointsRequired;
      endingPercent = endingPoints / pointsRequired;

    }
    else {
      // if we are at max level, just play the max level animation (leaves the end dot full)
      startingPercent = endingPercent = 1f;

      SetFriendshipTexts(
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel - 1].FriendshipLevelName,
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel].FriendshipLevelName);
    }

    yield return StartCoroutine(FriendshipBarFill(startingPercent, endingPercent, kFillDuration));
    if (FriendshipBarAnimateComplete != null) {
      FriendshipBarAnimateComplete(data, this);
    }
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

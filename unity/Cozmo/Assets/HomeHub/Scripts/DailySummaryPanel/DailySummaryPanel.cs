using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DataPersistence;
using Cozmo.UI;
using Anki.Cozmo;

public class DailySummaryPanel : BaseView {

  // Animation Names
  private const string kIdleAnimationName = "Idle";
  private const string kLevelReachedAnimationName = "LevelReached";
  private const string kMaxLevelAnimationName = "MaxLevel";

  // B versions put the current level label on the bottom
  private const string kIdleAnimationNameB = "IdleB";
  private const string kLevelReachedAnimationNameB = "LevelReachedB";
  private const string kMaxLevelAnimationNameB = "MaxLevelB";

  public Cozmo.HomeHub.TimelineView.OnFriendshipBarAnimateComplete FriendshipBarAnimateComplete;

  // yay magic numbers
  const float kInitialFillPoint = 0.43f;
  const float kFinalFillPoint = 0.835f;
  const float kNextFillPoint = 1.00f;
  const float kFillDuration = 1.5f;

  [SerializeField]
  private AnkiTextLabel _Title;

  [SerializeField]
  private Animator _FriendBarAnimator;

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
  private FriendshipFormulaConfiguration _FriendshipFormulaConfig;
  private FriendshipProgressionConfig _Config;

  protected override void CleanUp() {

  }

  public void Initialize(TimelineEntryData data) {
    int day = data.Date.Day;
    int month = data.Date.Month;

    _Title.FormattingArgs = new object[] { month, day };
    _BonusBarPanel = UIManager.CreateUIElement(_BonusBarPrefab.gameObject, _BonusBarContainer).GetComponent<BonusBarPanel>();

    float dailyProg = _FriendshipFormulaConfig.CalculateDailyGoalProgress(data.Progress, data.Goals);
    _DailyProgressBar.SetProgress(dailyProg);
    _BonusBarPanel.SetFriendshipBonus(dailyProg);

    for (int i = 0; i < (int)ProgressionStatType.Count; i++) {
      var stat = (ProgressionStatType)i;
      if (data.Goals[stat] > 0) {
        CreateGoalCell(stat, data.Goals[stat], data.Progress[stat]);
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
  private GoalCell CreateGoalCell(ProgressionStatType type, int goal, int progress) {
    GoalCell newBadge = UIManager.CreateUIElement(_ObjectivePrefab.gameObject, _ObjectivesContainer).GetComponent<GoalCell>();
    newBadge.Initialize(type, goal, progress, false);
    newBadge.ShowText(true);
    return newBadge;
  }

  private ChallengeBadge CreateChallengeBadge(string challengeId, RectTransform container) {
    ChallengeBadge newBadge = UIManager.CreateUIElement(_ChallengePrefab.gameObject, container).GetComponent<ChallengeBadge>();
    newBadge.Initialize(challengeId);
    return newBadge;
  }

  private IEnumerator FriendshipBarFill(float start, float end, float timeLimit, bool isNext = false) {
    float fillEnd = isNext ? kNextFillPoint : kFinalFillPoint;
    start = Mathf.Lerp(kInitialFillPoint, fillEnd, start);
    end = Mathf.Lerp(kInitialFillPoint, fillEnd, end);

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
      // play the idle which displays the labels we want
      _FriendBarAnimator.Play(level % 2 == 0 ? kIdleAnimationName : kIdleAnimationNameB);

      // set up the labels
      SetFriendshipTexts(
        levelConfig.FriendshipLevels[level].FriendshipLevelName,
        levelConfig.FriendshipLevels[level + 1].FriendshipLevelName,
        level % 2 == 0,
        level + 2 < levelConfig.FriendshipLevels.Length ?
          levelConfig.FriendshipLevels[level + 2].FriendshipLevelName :
          string.Empty);

      pointsRequired = levelConfig.FriendshipLevels[level + 1].PointsRequired;

      startingPercent = startingPoints / pointsRequired;

      // fill the progress bar to the top
      bool nextLvl = data.EndingFriendshipLevel - level >= 1.0f;
      yield return StartCoroutine(FriendshipBarFill(startingPercent, 1f, kFillDuration / (data.EndingFriendshipLevel - level), nextLvl));

      // if there is another level after the next, play the animation
      // where the next dot slides to where the current dot is
      if (level + 2 < levelConfig.FriendshipLevels.Length) {
        _FriendBarAnimator.Play(level % 2 == 0 ? kLevelReachedAnimationName : kLevelReachedAnimationNameB);

        yield return StartCoroutine(FriendshipBarFill(1, 0, 0.0f));
      }
      else {
        // otherwise, we are at max level. fill the next circle and keep it there
        _FriendBarAnimator.Play(level % 2 == 0 ? kMaxLevelAnimationName : kMaxLevelAnimationNameB);
        yield break;
      }

      // our next fill will start at point 0
      startingPoints = 0;
    }

    // after filling all completed levels, we want to animate our progress on the current level
    int endingPoints = data.EndingFriendshipPoints;

    // if this isn't the max level, show our progress toward our next level
    if (data.EndingFriendshipLevel + 1 < levelConfig.FriendshipLevels.Length) {

      _FriendBarAnimator.Play(data.EndingFriendshipLevel % 2 == 0 ? kIdleAnimationName : kIdleAnimationNameB);

      SetFriendshipTexts(
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel].FriendshipLevelName,
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel + 1].FriendshipLevelName,
        data.EndingFriendshipLevel % 2 == 0);

      pointsRequired = levelConfig.FriendshipLevels[data.EndingFriendshipLevel + 1].PointsRequired;

      startingPercent = startingPoints / pointsRequired;
      endingPercent = endingPoints / pointsRequired;

    }
    else {
      // if we are at max level, just play the max level animation (leaves the end dot full)
      startingPercent = endingPercent = 1f;

      _FriendBarAnimator.Play(data.EndingFriendshipLevel % 2 == 1 ? kMaxLevelAnimationName : kMaxLevelAnimationNameB);

      SetFriendshipTexts(
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel - 1].FriendshipLevelName,
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel].FriendshipLevelName,
        data.EndingFriendshipLevel % 2 == 1);
    }

    yield return StartCoroutine(FriendshipBarFill(startingPercent, endingPercent, kFillDuration));
    if (FriendshipBarAnimateComplete != null) {
      FriendshipBarAnimateComplete(data, this);
    }
  }

  // sets the labels on the friendship timeline
  private void SetFriendshipTexts(string current, string next, bool even, string nextNext = null) {

    // not sure what setting text to null does
    nextNext = nextNext ?? string.Empty;

    // for even levels, we show the top label for the current and the bottom for the next
    // we set the unshown labels to the next value, so when the animation ends and
    // switches them, they will already be correct regardless of synchronization with code
    _CurrentFriendshipLevel.text = current;
    _NextFriendshipLevel.text = next;
  }
}

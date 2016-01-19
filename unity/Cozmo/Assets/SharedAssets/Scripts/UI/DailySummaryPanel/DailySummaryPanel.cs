using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DataPersistence;
using Cozmo.UI;
using Anki.Cozmo;

public class DailySummaryPanel : MonoBehaviour {

  private const string kIdleAnimationName = "Idle";

  private const string kLevelReachedAnimationName = "LevelReached";

  private const string kMaxLevelAnimationName = "MaxLevel";

  private const string kIdleAnimationNameB = "IdleB";

  private const string kLevelReachedAnimationNameB = "LevelReachedB";

  private const string kMaxLevelAnimationNameB = "MaxLevelB";



  [SerializeField]
  private Button _CloseButton;

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
  private AnkiTextLabel _CurrentFriendshipLevelTop;
  [SerializeField]
  private AnkiTextLabel _CurrentFriendshipLevelBottom;

  [SerializeField]
  private AnkiTextLabel _NextFriendshipLevelTop;
  [SerializeField]
  private AnkiTextLabel _NextFriendshipLevelBottom;

  [SerializeField]
  private AnkiTextLabel _NextNextFriendshipLevelTop;
  [SerializeField]
  private AnkiTextLabel _NextNextFriendshipLevelBottom;


  private void Awake() {
    _CloseButton.onClick.AddListener(HandleCloseClicked);
  }

  private void HandleCloseClicked() {
    // Probably want to do something better than this.
    GameObject.Destroy(gameObject);
  }

  public void Initialize(TimelineEntryData data) {
    int day = data.Date.Day;
    int month = data.Date.Month;

    _Title.text = string.Format(_Title.DisplayText, month, day);

    _DailyProgressBar.SetProgress(data.Progress.Total / (float)data.Goals.Total);

    for (int i = 0; i < (int)ProgressionStatType.Count; i++) {
      var stat = (ProgressionStatType)i;
      if (data.Goals[stat] > 0) {
        CreateGoalBadge(stat, data.Goals[stat], data.Progress[stat]);
      }
    }

    RectTransform subContainer = null;
    for (int i = 0; i < data.CompletedChallenges.Count; i++) {
      // we want challenges to appear in groups of 3
      if (i % 3 == 0) {
        var subContainerObject = new GameObject();

        var layoutGroup = subContainerObject.AddComponent<HorizontalLayoutGroup>();
        layoutGroup.childForceExpandWidth = true;
        layoutGroup.childForceExpandHeight = false;

        // not sure if subContainer will automatically have a rect transform
        subContainer = subContainerObject.GetComponent<RectTransform>() ?? subContainerObject.AddComponent<RectTransform>();
        subContainer.SetParent(_ChallengesContainer, false);
      }
      CreateChallengeBadge(data.CompletedChallenges[i].ChallengeId, subContainer);
    }

    StartCoroutine(FriendshipBarCoroutine(data));
  }

  // Creates a goal badge
  private GoalBadge CreateGoalBadge(ProgressionStatType type, int goal, int progress) {
    GoalBadge newBadge = UIManager.CreateUIElement(_ObjectivePrefab.gameObject, _ObjectivesContainer).GetComponent<GoalBadge>();
    newBadge.Initialize(type.ToString(),goal,progress);
    newBadge.Expand(true);
    return newBadge;
  }

  private ChallengeBadge CreateChallengeBadge(string challengeId, RectTransform container) {
    ChallengeBadge newBadge = UIManager.CreateUIElement(_ChallengePrefab.gameObject, container).GetComponent<ChallengeBadge>();
    newBadge.Initialize(challengeId);
    return newBadge;
  }

  private IEnumerator FriendshipBarFill(float start, float end) {
    // this is pretty bad
    const float initialPoint = 0.14f;
    const float finalPoint = 0.86f;
    const float timeLimit = 1.42f;

    start = Mathf.Lerp(initialPoint, finalPoint, start);
    end = Mathf.Lerp(initialPoint, finalPoint, end);

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

    var levelConfig = RobotEngineManager.Instance.GetFriendshipLevelConfig();

    int startingPoints = data.StartingFriendshipPoints;
    float pointsRequired, startingPercent, endingPercent;
    for(int level = data.StartingFriendshipLevel; 
          level < data.EndingFriendshipLevel; level++) {
      _FriendBarAnimator.Play(level % 2 == 0 ? kIdleAnimationName : kIdleAnimationNameB);

      SetFriendshipTexts(
        levelConfig.FriendshipLevels[level].FriendshipLevelName,
        levelConfig.FriendshipLevels[level + 1].FriendshipLevelName,
        level % 2 == 0,
        level + 2 < levelConfig.FriendshipLevels.Length ?
          levelConfig.FriendshipLevels[level + 2].FriendshipLevelName :
          string.Empty);

      pointsRequired = levelConfig.FriendshipLevels[level + 1].PointsRequired;

      startingPercent = startingPoints / pointsRequired;

      yield return StartCoroutine(FriendshipBarFill(startingPercent, 1f));

      if (level + 2 < levelConfig.FriendshipLevels.Length) {
        _FriendBarAnimator.Play(level % 2 == 0 ? kLevelReachedAnimationName : kLevelReachedAnimationNameB);

        yield return StartCoroutine(FriendshipBarFill(1, 0));
      }
      else {
        _FriendBarAnimator.Play(level % 2 == 0 ? kMaxLevelAnimationName : kMaxLevelAnimationNameB);
        yield break;
      }

      startingPoints = 0;
    }

    int endingPoints = data.EndingFriendshipPoints;

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
      startingPercent = endingPercent = 1f;

      _FriendBarAnimator.Play(data.EndingFriendshipLevel % 2 == 1 ? kMaxLevelAnimationName : kMaxLevelAnimationNameB);

      SetFriendshipTexts(
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel - 1].FriendshipLevelName,
        levelConfig.FriendshipLevels[data.EndingFriendshipLevel].FriendshipLevelName,
        data.EndingFriendshipLevel % 2 == 1);
    }

    yield return StartCoroutine(FriendshipBarFill(startingPercent, endingPercent));
  }

  private void SetFriendshipTexts(string current, string next, bool even, string nextNext = null) {

    nextNext = nextNext ?? string.Empty;
    // TODO: Localize
    _CurrentFriendshipLevelTop.text = even ? current : next;
    _CurrentFriendshipLevelBottom.text = even ? next : current;
    _NextFriendshipLevelTop.text = even ? nextNext : next;
    _NextFriendshipLevelBottom.text = even ? next : nextNext;
    _NextNextFriendshipLevelBottom.text = nextNext;
    _NextNextFriendshipLevelTop.text = nextNext;
  }
}

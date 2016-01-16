using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DataPersistence;
using Cozmo.UI;
using Anki.Cozmo;

public class DailySummaryPanel : MonoBehaviour {

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
}

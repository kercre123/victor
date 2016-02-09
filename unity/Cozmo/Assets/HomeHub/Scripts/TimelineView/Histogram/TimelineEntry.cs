using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System;
using DataPersistence;
using Anki.UI;

public class TimelineEntry : MonoBehaviour {

  [SerializeField]
  private Image _Fillbar;

  [SerializeField]
  private Image _TimelineNodeBonus;

  [SerializeField]
  private GameObject _TimelineNodeActive;

  [SerializeField]
  private GameObject _TimelineNodeComplete;

  [SerializeField]
  private GameObject _TimelineNodeFirst;

  [SerializeField]
  private GameObject _TimelineNodeInactive;

  [SerializeField]
  private Button _FillbarButton;

  [SerializeField]
  private Button _TimelineNodeButton;

  [SerializeField]
  private GameObject _FriendshipMilestone;

  [SerializeField]
  private AnkiTextLabel _FriendshipLabel;

  [SerializeField]
  private GameObject _WeekLine;

  [SerializeField]
  private AnkiTextLabel _WeekLabel;
  [SerializeField]
  private AnkiTextLabel _WeekLabelGlow;

  private Date _Date;

  public event Action<Date> OnSelect;

  private void Awake() {
    _FillbarButton.onClick.AddListener(HandleClick);
    _TimelineNodeButton.onClick.AddListener(HandleClick);
  }

  private void HandleClick() {
    if (OnSelect != null) {
      OnSelect(_Date);
    }
  }

  public void Inititialize(Date date, TimelineEntryData timelineEntry, float progress, bool first, bool showWeek, int week) {
    _Date = date;
    _Fillbar.fillAmount = progress;

    bool active = timelineEntry != null;

    _FriendshipMilestone.SetActive(false);
    if (timelineEntry != null) {
      if (timelineEntry.EndingFriendshipLevel > timelineEntry.StartingFriendshipLevel) {
        _FriendshipMilestone.SetActive(true);
        var levelConfig = DailyGoalManager.Instance.GetFriendshipProgressConfig();
        _FriendshipLabel.text = levelConfig.FriendshipLevels[timelineEntry.EndingFriendshipLevel].FriendshipLevelName;
      }
      SetFriendshipBonus(DailyGoalManager.Instance.CalculateBonusMult(timelineEntry.Progress, timelineEntry.Goals));
    }

    _FillbarButton.gameObject.SetActive(active);
    _TimelineNodeActive.SetActive(active && progress < 1.0f && !first);
    _TimelineNodeComplete.SetActive(progress >= 1.0f && !first);
    _TimelineNodeInactive.SetActive(!active && !first);
    _TimelineNodeFirst.SetActive(first);

    _WeekLine.SetActive(showWeek);
    if (showWeek) {
      _WeekLabelGlow.FormattingArgs = _WeekLabel.FormattingArgs = new object[]{ week };
    }
  }

  // Show "completed" gems only in Timeline Entry
  private void SetFriendshipBonus(float prog) {
    int mult = Mathf.CeilToInt(prog);
    int multIndex = mult - 2;
    _TimelineNodeBonus.gameObject.SetActive(false);
    if (multIndex < 0) {
      return;
    }

    var friendshipConfig = DailyGoalManager.Instance.GetFriendshipProgressConfig();

    if (friendshipConfig.BonusMults.Length == 0) {
      return;
    }

    if (multIndex >= friendshipConfig.BonusMults.Length) {
      multIndex = friendshipConfig.BonusMults.Length - 1;
    }

    _TimelineNodeBonus.gameObject.SetActive(true);
    _TimelineNodeBonus.sprite = friendshipConfig.BonusMults[multIndex].Complete;
  }
}

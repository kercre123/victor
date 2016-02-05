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
  private GameObject _TimelineNodeBonus;

  [SerializeField]
  private GameObject _TimelineNodeActive;

  [SerializeField]
  private GameObject _TimelineNodeComplete;

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

  public void Inititialize(Date date, TimelineEntryData timelineEntry, float progress) {
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
    }

    _FillbarButton.gameObject.SetActive(active);
    _TimelineNodeActive.SetActive(active && progress < 1.0f);
    _TimelineNodeComplete.SetActive(progress >= 1.0f);
    _TimelineNodeInactive.SetActive(!active);

    if (timelineEntry != null) {      
      _TimelineNodeBonus.SetActive(progress > 1.0f);
    }
  }
}

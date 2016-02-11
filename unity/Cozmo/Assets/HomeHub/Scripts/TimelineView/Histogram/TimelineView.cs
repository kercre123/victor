using System;
using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;
using System.Linq;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo.HomeHub {
  public class TimelineView : BaseView {

    public static int kGeneratedTimelineHistoryLength = 21;

    [SerializeField]
    private float _TimelineStartOffset = 150f;

    [SerializeField]
    private float _MaxDailyGoalWidth = 800f;

    [SerializeField]
    private float _MinDailyGoalWidth = 400f;

    [SerializeField]
    private float _RightDailyGoalOffset = 150f;

    [SerializeField]
    private GameObject _TimelineEntryPrefab;

    private readonly List<TimelineEntry> _TimelineEntries = new List<TimelineEntry>();

    [SerializeField]
    private RectTransform _TimelineContainer;

    [SerializeField]
    private RectTransform _ChallengeContainer;

    [SerializeField]
    private RectTransform _DailyGoalsContainer;

    [SerializeField]
    private RectTransform _CozmoWidgetContainer;

    [SerializeField]
    private RectTransform _LockedPaneScrollContainer;

    [SerializeField]
    private RectTransform _LockedPaneRightNoScrollContainer;

    [SerializeField]
    private RectTransform _LockedPaneLeftNoScrollContainer;

    [SerializeField]
    private UnityEngine.UI.ScrollRect _ScrollRect;

    [SerializeField]
    private RectTransform _ContentPane;

    [SerializeField]
    private RectTransform _ViewportPane;

    [SerializeField]
    private RectTransform _TimelinePane;

    [SerializeField]
    private LayoutElement _MiddlePane;

    [SerializeField]
    private AnkiButton _EndSessionButton;

    public delegate void OnFriendshipBarAnimateComplete(TimelineEntryData data,DailySummaryPanel summaryPanel);

    public delegate void ButtonClickedHandler(string challengeClicked,Transform buttonTransform);

    public event ButtonClickedHandler OnLockedChallengeClicked;
    public event ButtonClickedHandler OnUnlockedChallengeClicked;
    public event ButtonClickedHandler OnCompletedChallengeClicked;


    public event Action OnEndSessionClicked;

    [SerializeField]
    HomeHubChallengeListView _ChallengeListViewPrefab;
    HomeHubChallengeListView _ChallengeListViewInstance;

    [SerializeField]
    DailyGoalPanel _DailyGoalPrefab;
    DailyGoalPanel _DailyGoalInstance;

    [SerializeField]
    DailySummaryPanel _DailySummaryPrefab;

    [SerializeField]
    CozmoWidget _CozmoWidgetPrefab;
    CozmoWidget _CozmoWidgetInstance;

    private bool _ScrollLocked;
    private float _ScrollLockedOffset;

    protected override void CleanUp() {
      _ScrollRect.onValueChanged.RemoveAllListeners();
    }

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      _ChallengeListViewInstance = UIManager.CreateUIElement(_ChallengeListViewPrefab.gameObject, _ChallengeContainer).GetComponent<HomeHubChallengeListView>();
      _ChallengeListViewInstance.Initialize(challengeStatesById);
      _ChallengeListViewInstance.OnLockedChallengeClicked += OnLockedChallengeClicked;
      _ChallengeListViewInstance.OnUnlockedChallengeClicked += OnUnlockedChallengeClicked;

      _DailyGoalInstance = UIManager.CreateUIElement(_DailyGoalPrefab.gameObject, _DailyGoalsContainer).GetComponent<DailyGoalPanel>();

      _CozmoWidgetInstance = UIManager.CreateUIElement(_CozmoWidgetPrefab.gameObject, _CozmoWidgetContainer).GetComponent<CozmoWidget>();

      UpdateDailySession();

      PopulateTimeline(DataPersistenceManager.Instance.Data.Sessions);
      _ContentPane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;
      _TimelinePane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;

      _EndSessionButton.onClick.AddListener(HandleEndSessionButtonTap);
      Robot currentRobot = RobotEngineManager.Instance.CurrentRobot;
      _CozmoWidgetInstance.UpdateFriendshipText(currentRobot.GetFriendshipLevelName(currentRobot.FriendshipLevel));

      // Locking and expanding daily goals init
      _ScrollRect.onValueChanged.AddListener(HandleTimelineViewScroll);
      _MiddlePane.minWidth = _MaxDailyGoalWidth; 
    }

    private void PopulateTimeline(List<TimelineEntryData> timelineEntries) {
      int timelineIndex = 0;

      var today = DataPersistenceManager.Today;

      var startDate = today.AddDays(-kGeneratedTimelineHistoryLength);

      var firstSession = DataPersistenceManager.Instance.Data.Sessions.FirstOrDefault();

      bool showFirst = false;

      if (firstSession != null) {
        if (startDate < firstSession.Date) {
          startDate = firstSession.Date.AddDays(-1);
          showFirst = true;
        }
      }
      else {
        startDate = today.AddDays(-1);
        showFirst = true;
      }

      while (timelineEntries.Count > 0 && timelineEntries[0].Date < startDate) {
        timelineEntries.RemoveAt(0);
      }

      for (int i = 0; i < kGeneratedTimelineHistoryLength; i++) {
        var spawnedObject = UIManager.CreateUIElement(_TimelineEntryPrefab, _TimelineContainer);

        var date = startDate.AddDays(i);
        var entry = spawnedObject.GetComponent<TimelineEntry>();

        TimelineEntryData timelineEntry = null;
        float progress = 0f;
        if (timelineIndex < timelineEntries.Count && timelineEntries[timelineIndex].Date.Equals(date)) {
          timelineEntry = timelineEntries[timelineIndex];
          progress = DailyGoalManager.Instance.CalculateDailyGoalProgress(timelineEntry.Progress, timelineEntry.Goals);
          timelineIndex++;
        }

        int daysAgo = (today - date).Days;

        entry.Inititialize(date, timelineEntry, progress, i == 0 && showFirst, daysAgo % 7 == 0, daysAgo / 7);
        showFirst = false;

        entry.OnSelect += HandleTimelineEntrySelected;

        _TimelineEntries.Add(entry);
      }
    }

    public void LockScroll(bool locked) {
      StartCoroutine(LockScrollCoroutine(locked));
    }

    private IEnumerator LockScrollCoroutine(bool locked) {
      //Scroll all the way to the left, then move the scroll container.
      _ScrollLocked = false;

      // add a .2 second delay on closing
      float progress = (locked ? 0 : 1.4f);

      float startingHorizontalOffset = locked ? _ScrollRect.horizontalNormalizedPosition : _ScrollLockedOffset;
      _ScrollLockedOffset = startingHorizontalOffset;

      float contentWidth = _ContentPane.rect.width - _ViewportPane.rect.width;

      float screenWidth = _ViewportPane.rect.width;

      float offset = startingHorizontalOffset * contentWidth;

      float pixelsToMove = contentWidth - offset + screenWidth;

      while ((progress < 1f && locked) || (progress > 0f && !locked)) {
        progress = progress + (locked ? 2 : -2) * Time.deltaTime;

        float position = offset + Mathf.Lerp(0, pixelsToMove, Mathf.Clamp01(progress));

        if (position <= contentWidth) {
          _ScrollRect.horizontalNormalizedPosition = position / contentWidth;
          _ScrollRect.transform.localPosition = Vector3.zero;
        }
        else {
          _ScrollRect.transform.localPosition = Vector2.left * (position - contentWidth);
        }
         
        HandleTimelineViewScroll(Vector2.zero);

        yield return null;
      }

      _ScrollLocked = locked;

    }

    private void UpdateDailySession() {
      var currentSession = DataPersistenceManager.Instance.CurrentSession;
      Robot currentRobot = RobotEngineManager.Instance.CurrentRobot;
      // check if the current session is still valid
      if (currentSession != null) {  
        _DailyGoalInstance.SetDailyGoals(currentSession.Progress, currentSession.Goals);
        float currNeed = DailyGoalManager.Instance.GetMinigameNeed_Extremes();
        currentRobot.AddToEmotion(Anki.Cozmo.EmotionType.WantToPlay, currNeed, "DailyGoalProgress");
        DailyGoalManager.Instance.PickMiniGameToRequest();
        return;
      }

      var lastSession = DataPersistenceManager.Instance.Data.Sessions.LastOrDefault();

      if (lastSession != null && !lastSession.Complete) {
        CompleteSession(lastSession);
      }

      // start a new session
      StatContainer goals = DailyGoalManager.Instance.GenerateDailyGoals();

      TimelineEntryData newSession = new TimelineEntryData(DataPersistenceManager.Today) {
        StartingFriendshipLevel = RobotEngineManager.Instance.CurrentRobot.FriendshipLevel,
        StartingFriendshipPoints = RobotEngineManager.Instance.CurrentRobot.FriendshipPoints
      };

      newSession.Goals.Set(goals);

      currentRobot.SetProgressionStats(newSession.Progress);
      _DailyGoalInstance.SetDailyGoals(newSession.Progress, newSession.Goals);

      DataPersistenceManager.Instance.Data.Sessions.Add(newSession);

      DataPersistenceManager.Instance.Save();
    }

    private void CompleteSession(TimelineEntryData timelineEntry) {

      int friendshipPoints = DailyGoalManager.Instance.CalculateFriendshipPoints(timelineEntry.Progress, timelineEntry.Goals);

      RobotEngineManager.Instance.CurrentRobot.AddToFriendshipPoints(friendshipPoints);
      UpdateFriendshipPoints(timelineEntry, friendshipPoints);
      ShowDailySessionPanel(timelineEntry, HandleOnFriendshipBarAnimateComplete);
    }

    private void UpdateFriendshipPoints(TimelineEntryData timelineEntry, int friendshipPoints) {
      timelineEntry.AwardedFriendshipPoints = friendshipPoints;
      DataPersistenceManager.Instance.Data.FriendshipLevel
      = timelineEntry.EndingFriendshipLevel
        = RobotEngineManager.Instance.CurrentRobot.FriendshipLevel;
      DataPersistenceManager.Instance.Data.FriendshipPoints
      = timelineEntry.EndingFriendshipPoints
        = RobotEngineManager.Instance.CurrentRobot.FriendshipPoints;
      timelineEntry.Complete = true;
    }

    private void HandleTimelineEntrySelected(Date date) {
      var session = DataPersistenceManager.Instance.Data.Sessions.Find(x => x.Date == date);

      if (session != null) {
        ShowDailySessionPanel(session);
      }
    }

    private void ShowDailySessionPanel(TimelineEntryData session, OnFriendshipBarAnimateComplete onComplete = null) {
      var summaryPanel = UIManager.OpenView(_DailySummaryPrefab, transform).GetComponent<DailySummaryPanel>();
      summaryPanel.Initialize(session);
      if (onComplete != null) {
        summaryPanel.FriendshipBarAnimateComplete += onComplete;
      }
    }

    private void HandleOnFriendshipBarAnimateComplete(TimelineEntryData data, DailySummaryPanel summaryPanel) {
      TimeSpan deltaTime = (DataPersistenceManager.Instance.Data.Sessions[DataPersistenceManager.Instance.Data.Sessions.Count - 2].Date - DataPersistenceManager.Today);
      int friendshipPoints = ((int)deltaTime.TotalDays + 1) * 10;
      summaryPanel.FriendshipBarAnimateComplete -= HandleOnFriendshipBarAnimateComplete;

      if (friendshipPoints < 0) {
        RobotEngineManager.Instance.CurrentRobot.AddToFriendshipPoints(friendshipPoints);
        UpdateFriendshipPoints(data, friendshipPoints);
        summaryPanel.AnimateFriendshipBar(data);
      }
    }

    #region Daily Goal locking

    private void SetScrollRectStartPosition() {
      _ScrollRect.horizontalNormalizedPosition = 
        CalculateHorizontalNormalizedPosition(_TimelinePane.rect.width - _TimelineStartOffset);
      SetNoScrollContainerWidth(_LockedPaneRightNoScrollContainer, _MinDailyGoalWidth, -_RightDailyGoalOffset);
      SetNoScrollContainerWidth(_LockedPaneLeftNoScrollContainer, _MaxDailyGoalWidth, 0);
    }

    private void HandleTimelineViewScroll(Vector2 scrollPosition) {
      if (_ScrollLocked) {
        return;
      }

      float currentScrollPosition = _ScrollRect.horizontalNormalizedPosition;
      if (currentScrollPosition <= GetDailyGoalOnRightHorizontalNormalizedPosition()) {
        if (_LockedPaneScrollContainer.parent != _LockedPaneRightNoScrollContainer) {
          _LockedPaneScrollContainer.SetParent(_LockedPaneRightNoScrollContainer, false);
          _LockedPaneScrollContainer.localPosition = Vector3.zero; 
          _DailyGoalInstance.Collapse();
          _LockedPaneRightNoScrollContainer.gameObject.SetActive(true);
        }
      }
      else if (currentScrollPosition >= GetDailyGoalOnLeftHorizontalNormalizedPosition()) {
        if (_LockedPaneScrollContainer.parent != _LockedPaneLeftNoScrollContainer) {
          _LockedPaneScrollContainer.SetParent(_LockedPaneLeftNoScrollContainer, false);
          _LockedPaneScrollContainer.localPosition = Vector3.zero; 
          _DailyGoalInstance.Collapse();
          _LockedPaneLeftNoScrollContainer.gameObject.SetActive(true);
        }
        float pixelsOnLeft = GetScrollRectNormalizedWidth() * _ScrollRect.horizontalNormalizedPosition;
        float dailyGoalPaneOverhang = pixelsOnLeft - _TimelinePane.rect.width;
        float dailyGoalDisplayWidth = _MiddlePane.minWidth - dailyGoalPaneOverhang;
        dailyGoalDisplayWidth = Mathf.Clamp(dailyGoalDisplayWidth, _MinDailyGoalWidth, _MaxDailyGoalWidth);
        SetNoScrollContainerWidth(_LockedPaneLeftNoScrollContainer, dailyGoalDisplayWidth, 0);
      }
      else {
        if (_LockedPaneScrollContainer.parent != _MiddlePane) {
          _LockedPaneScrollContainer.SetParent(_MiddlePane.transform, false);
          _LockedPaneScrollContainer.localPosition = Vector3.zero; 
          _DailyGoalInstance.Expand();
          _LockedPaneRightNoScrollContainer.gameObject.SetActive(false);
          _LockedPaneLeftNoScrollContainer.gameObject.SetActive(false);
        }
      }
    }

    private float GetDailyGoalOnLeftHorizontalNormalizedPosition() {
      return _TimelinePane.rect.width / GetScrollRectNormalizedWidth();
    }

    private float GetDailyGoalOnRightHorizontalNormalizedPosition() {
      return (_TimelinePane.rect.width + _RightDailyGoalOffset - _ViewportPane.rect.width)
      / GetScrollRectNormalizedWidth();
    }

    private float CalculateHorizontalNormalizedPosition(float pixelsToLeftOfScreen) {
      return Mathf.Clamp01(pixelsToLeftOfScreen / GetScrollRectNormalizedWidth());
    }

    private float GetScrollRectNormalizedWidth() {
      return (_ContentPane.rect.width - _ViewportPane.rect.width);
    }

    private void SetNoScrollContainerWidth(RectTransform container, float width, float xOffset) {
      container.sizeDelta = new Vector2(width, 0);
      container.localPosition = new Vector3(width * 0.5f + xOffset, 0, 0);
    }

    #endregion

    #region End Session

    private void HandleEndSessionButtonTap() {
      // Open confirmation dialog instead
      AlertView alertView = UIManager.OpenView(UIPrefabHolder.Instance.AlertViewPrefab) as AlertView;
      // Hook up callbacks
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleEndSessionConfirm);
      alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleEndSessionCancel);
      alertView.TitleLocKey = LocalizationKeys.kEndSessionTitle;
      alertView.DescriptionLocKey = LocalizationKeys.kEndSessionDescription;
    }

    private void HandleEndSessionCancel() {
      DAS.Info(this, "HandleEndSessionCancel");
    }

    private void HandleEndSessionConfirm() {
      DAS.Info(this, "HandleEndSessionConfirm");

      if (OnEndSessionClicked != null) {
        OnEndSessionClicked();
      }
    }

    #endregion
  }
}


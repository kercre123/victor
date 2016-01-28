using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;
using System.Linq;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo.HomeHub {
  public class TimelineView : BaseView {

    public static int kTimelineHistoryLength = 14;

    [SerializeField]
    private GameObject _TimelineEntryPrefab;

    private readonly List<TimelineEntry> _TimelineEntries = new List<TimelineEntry>();

    [SerializeField]
    private RectTransform _TimelineContainer;

    [SerializeField]
    private RectTransform _ChallengeContainer;

    [SerializeField]
    private RectTransform _RightTopPane;

    [SerializeField]
    private RectTransform _RightBottomPane;

    [SerializeField]
    private UnityEngine.UI.ScrollRect _ScrollRect;

    [SerializeField]
    private RectTransform _ContentPane;

    [SerializeField]
    private RectTransform _TimelinePane;

    [SerializeField]
    private GraphSpline _GraphSpline;

    [SerializeField]
    private AnkiButton _EndSessionButton;

    public delegate void OnFriendshipBarAnimateComplete(TimelineEntryData data,DailySummaryPanel summaryPanel);

    public delegate void ButtonClickedHandler(string challengeClicked,Transform buttonTransform);

    public event ButtonClickedHandler OnLockedChallengeClicked;
    public event ButtonClickedHandler OnUnlockedChallengeClicked;
    public event ButtonClickedHandler OnCompletedChallengeClicked;

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

    [SerializeField]
    FriendshipFormulaConfiguration _FriendshipFormulaConfig;

    protected override void CleanUp() {
    }

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      _ChallengeListViewInstance = UIManager.CreateUIElement(_ChallengeListViewPrefab.gameObject, _ChallengeContainer).GetComponent<HomeHubChallengeListView>();
      _ChallengeListViewInstance.Initialize(challengeStatesById);
      _ChallengeListViewInstance.OnLockedChallengeClicked += OnLockedChallengeClicked;
      _ChallengeListViewInstance.OnUnlockedChallengeClicked += OnUnlockedChallengeClicked;

      _DailyGoalInstance = UIManager.CreateUIElement(_DailyGoalPrefab.gameObject, _RightTopPane).GetComponent<DailyGoalPanel>();

      _CozmoWidgetInstance = UIManager.CreateUIElement(_CozmoWidgetPrefab.gameObject, _RightBottomPane).GetComponent<CozmoWidget>();

      UpdateDailySession();

      PopulateTimeline(DataPersistenceManager.Instance.Data.Sessions);
      _ContentPane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;
      _TimelinePane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;

      _DailyGoalInstance.transform.SetSiblingIndex(0);
      _EndSessionButton.onClick.AddListener(HandleEndSessionButtonTap);
      Robot currentRobot = RobotEngineManager.Instance.CurrentRobot;
      _CozmoWidgetInstance.UpdateFriendshipText(currentRobot.GetFriendshipLevelName(currentRobot.FriendshipLevel));
    }

    private void UpdateDailySession() {

      var currentSession = DataPersistenceManager.Instance.CurrentSession;

      // check if the current session is still valid
      if (currentSession != null) {  
        _DailyGoalInstance.SetDailyGoals(currentSession.Goals, currentSession.Progress);
        return;
      }

      var lastSession = DataPersistenceManager.Instance.Data.Sessions.LastOrDefault();

      if (lastSession != null && !lastSession.Complete) {
        CompleteSession(lastSession);
      }

      // start a new session
      var goals = _DailyGoalInstance.GenerateDailyGoals();

      TimelineEntryData newSession = new TimelineEntryData(DataPersistenceManager.Today) {
        StartingFriendshipLevel = RobotEngineManager.Instance.CurrentRobot.FriendshipLevel,
        StartingFriendshipPoints = RobotEngineManager.Instance.CurrentRobot.FriendshipPoints
      };

      newSession.Goals.Set(goals);

      RobotEngineManager.Instance.CurrentRobot.SetProgressionStats(newSession.Progress);

      DataPersistenceManager.Instance.Data.Sessions.Add(newSession);

      DataPersistenceManager.Instance.Save();
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

    private void CompleteSession(TimelineEntryData timelineEntry) {

      int friendshipPoints = Mathf.RoundToInt(_FriendshipFormulaConfig.CalculateFriendshipScore(timelineEntry.Progress));

      RobotEngineManager.Instance.CurrentRobot.AddToFriendshipPoints(friendshipPoints);
      UpdateFriendshipPoints(timelineEntry, friendshipPoints);
      ShowDailySessionPanel(timelineEntry, HandleOnFriendshipBarAnimateComplete);
    }

    private void SetScrollRectStartPosition() {
      _ScrollRect.horizontalNormalizedPosition = _TimelinePane.rect.width / _ContentPane.rect.width;
    }

    private void PopulateTimeline(List<TimelineEntryData> timelineEntries) {
      int timelineIndex = 0;

      var today = DataPersistenceManager.Today;

      var startDate = today.AddDays(-kTimelineHistoryLength);

      while (timelineEntries.Count > 0 && timelineEntries[0].Date < startDate) {
        timelineEntries.RemoveAt(0);
      }

      List<float> graphPoints = new List<float>();
      for (int i = 0; i < kTimelineHistoryLength; i++) {
        var spawnedObject = UIManager.CreateUIElement(_TimelineEntryPrefab, _TimelineContainer);

        var date = startDate.AddDays(i);
        var entry = spawnedObject.GetComponent<TimelineEntry>();

        bool active = false;
        float progress = 0f;
        if (timelineIndex < timelineEntries.Count && timelineEntries[timelineIndex].Date.Equals(date)) {
          var state = timelineEntries[timelineIndex];
          progress = _FriendshipFormulaConfig.CalculateFriendshipProgress(state.Progress, state.Goals);
          active = true;
          timelineIndex++;
        }

        entry.Inititialize(date, active, progress);
        graphPoints.Add(progress);

        entry.OnSelect += HandleTimelineEntrySelected;

        _TimelineEntries.Add(entry);
      }

      _GraphSpline.Initialize(graphPoints);
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

    private void HandleEndSessionButtonTap() {
      // Open confirmation dialog instead
      AlertView alertView = UIManager.OpenView(UIPrefabHolder.Instance.AlertViewPrefab) as AlertView;
      // Hook up callbacks
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleEndSessionConfirm);
      alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleEndSessionCancel);
      alertView.TitleLocKey = LocalizationKeys.kEndSessionViewTitle;
      alertView.DescriptionLocKey = LocalizationKeys.kEndSessionViewDescription;
    }

    private void HandleEndSessionCancel() {
      DAS.Info(this, "HandleEndSessionCancel");
    }

    private void HandleEndSessionConfirm() {
      DAS.Info(this, "HandleEndSessionConfirm");
    }

  }
}


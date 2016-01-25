using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;
using System.Linq;

namespace Cozmo.HomeHub {
  public class TimelineView : MonoBehaviour {

    public static int kTimelineHistoryLength = 14;

    [SerializeField]
    private GameObject _TimelineEntryPrefab;

    private readonly List<TimelineEntry> _TimelineEntries = new List<TimelineEntry>();

    [SerializeField]
    private RectTransform _TimelineContainer;

    [SerializeField]
    private RectTransform _ChallengeContainer;

    [SerializeField]
    private UnityEngine.UI.ScrollRect _ScrollRect;

    [SerializeField]
    private RectTransform _ContentPane;

    [SerializeField]
    private RectTransform _TimelinePane;

    [SerializeField]
    private GraphSpline _GraphSpline;

    public delegate void ButtonClickedHandler(string challengeClicked, Transform buttonTransform);

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
    FriendshipFormulaConfiguration _FriendshipFormulaConfig;


    public void CloseView() {
      // TODO: Play some close animations before destroying view
      GameObject.Destroy(gameObject);
    }

    public void CloseViewImmediately() {
      GameObject.Destroy(gameObject);
    }

    public void OnDestroy() {
      if (_ChallengeListViewInstance != null) {
        GameObject.Destroy(_ChallengeListViewInstance.gameObject);
      }
    }

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      _ChallengeListViewInstance = UIManager.CreateUIElement(_ChallengeListViewPrefab.gameObject, _ChallengeContainer).GetComponent<HomeHubChallengeListView>();
      _ChallengeListViewInstance.Initialize(challengeStatesById);
      _ChallengeListViewInstance.OnLockedChallengeClicked += OnLockedChallengeClicked;
      _ChallengeListViewInstance.OnUnlockedChallengeClicked += OnUnlockedChallengeClicked;

      _DailyGoalInstance = UIManager.CreateUIElement(_DailyGoalPrefab.gameObject, _ContentPane).GetComponent<DailyGoalPanel>();

      UpdateDailySession();

      PopulateTimeline(DataPersistenceManager.Instance.Data.Sessions);
      _ContentPane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;
      _TimelinePane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;


      _DailyGoalInstance.transform.SetSiblingIndex(1);

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

    private void CompleteSession(TimelineEntryData timelineEntry) {

      int friendshipPoints = Mathf.RoundToInt(_FriendshipFormulaConfig.CalculateFriendshipScore(timelineEntry.Progress));

      RobotEngineManager.Instance.CurrentRobot.AddToFriendshipPoints(friendshipPoints);

      timelineEntry.AwardedFriendshipPoints = friendshipPoints;
      DataPersistenceManager.Instance.Data.FriendshipLevel
          = timelineEntry.EndingFriendshipLevel
          = RobotEngineManager.Instance.CurrentRobot.FriendshipLevel;
      DataPersistenceManager.Instance.Data.FriendshipPoints
          = timelineEntry.EndingFriendshipPoints
          = RobotEngineManager.Instance.CurrentRobot.FriendshipPoints;
      timelineEntry.Complete = true;

      ShowDailySessionPanel(timelineEntry);
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

    private void ShowDailySessionPanel(TimelineEntryData session) {
      var summaryPanel = UIManager.CreateUIElement(_DailySummaryPrefab, transform).GetComponent<DailySummaryPanel>();
      summaryPanel.Initialize(session);
    }

  }
}


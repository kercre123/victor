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


      // TMP: GENERATE FAKE DATA
      GenerateFakeData(challengeStatesById.Keys.ToArray());

      UpdateDailySession();


      PopulateTimeline(DataPersistenceManager.Instance.Data.Sessions);
      _ContentPane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;
      _TimelinePane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;


      _DailyGoalInstance.transform.SetSiblingIndex(1);

    }

    private void UpdateDailySession() {
      var lastDay = DataPersistenceManager.Instance.Data.Sessions.LastOrDefault();

      // check if the current session is still valid
      if (lastDay != null && lastDay.Date == DateTime.UtcNow.Date) {  
        _DailyGoalInstance.SetDailyGoals(lastDay.Goals, lastDay.Progress);
        return;
      }

      if (lastDay != null && !lastDay.Complete) {
        CompleteSession(lastDay);
      }

      // start a new session
      var goals = _DailyGoalInstance.GenerateDailyGoals();

      TimelineEntryData newSession = new TimelineEntryData(DateTime.UtcNow.Date) {
        StartingFriendshipLevel = RobotEngineManager.Instance.CurrentRobot.FriendshipLevel,
        StartingFriendshipPoints = RobotEngineManager.Instance.CurrentRobot.FriendshipPoints
      };

      newSession.Goals.Set(goals);

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

    // TMP!!!!
    private void GenerateFakeData(string[] challengeIds) {
      DataPersistenceManager.Instance.Data.Sessions.Clear();

      var today = DateTime.UtcNow.Date;

      var startDate = today.AddDays(-kTimelineHistoryLength);

      for (int i = 0; i < kTimelineHistoryLength; i++) {
        var date = startDate.AddDays(i);

        var entry = new TimelineEntryData(date);

        for (int j = 0; j < (int)Anki.Cozmo.ProgressionStatType.Count; j++) {
          var stat = (Anki.Cozmo.ProgressionStatType)j;
          if (UnityEngine.Random.Range(0f, 1f) > 0.6f) {
            int goal = UnityEngine.Random.Range(0, 6);
            int progress = Mathf.Clamp(UnityEngine.Random.Range(0, goal * 2), 0, goal);
            entry.Goals[stat] = goal;
            entry.Progress[stat] = progress;
          }
        }

        int challengeCount = UnityEngine.Random.Range(0, 20);

        for (int j = 0; j < challengeCount; j++) {
          entry.CompletedChallenges.Add(new CompletedChallengeData() {
            ChallengeId = challengeIds[UnityEngine.Random.Range(0, challengeIds.Length)]
          });
        }

        // don't add any days with goal of 0
        if (entry.Goals.Total > 0) {
          DataPersistenceManager.Instance.Data.Sessions.Add(entry);
        }
      }

    }

    private void PopulateTimeline(List<TimelineEntryData> timelineEntries) {
      int timelineIndex = 0;

      var today = DateTime.UtcNow.Date;

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
          progress = Mathf.Clamp01(state.Progress.Total / (float)state.Goals.Total);
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

    private void HandleTimelineEntrySelected(DateTime date) {
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


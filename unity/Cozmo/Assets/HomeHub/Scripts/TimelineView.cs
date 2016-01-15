using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;

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
      GenerateFakeData();

      _DailyGoalInstance.GenerateDailyGoals();

      PopulateTimeline(DataPersistenceManager.Instance.Data.PreviousSessions);
      _ContentPane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;
      _TimelinePane.GetComponent<RectChangedCallback>().OnRectChanged += SetScrollRectStartPosition;


      _DailyGoalInstance.transform.SetSiblingIndex(1);

    }

    private void SetScrollRectStartPosition() {
      _ScrollRect.horizontalNormalizedPosition = _TimelinePane.rect.width / _ContentPane.rect.width;
    }

    // TMP!!!!
    private void GenerateFakeData() {
      DataPersistenceManager.Instance.Data.PreviousSessions.Clear();

      var today = DateTime.UtcNow.Date;

      var startDate = today.AddDays(-kTimelineHistoryLength);

      for (int i = 0; i < kTimelineHistoryLength; i++) {
        var date = startDate.AddDays(i);

        if (UnityEngine.Random.Range(0f, 1f) > 0.3f) {
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

          DataPersistenceManager.Instance.Data.PreviousSessions.Add(entry);
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
      var session = DataPersistenceManager.Instance.Data.PreviousSessions.Find(x => x.Date == date);

      if (session != null) {
        // DO Something?
      }
    }


  }
}


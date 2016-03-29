using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using Cozmo.UI;
using DataPersistence;
using System.Linq;
using System;

namespace Cozmo.HomeHub {
  public class HomeView : BaseView {

    public System.Action<StatContainer, StatContainer, Transform[]> DailyGoalsSet;

    public delegate void OnFriendshipBarAnimateComplete(TimelineEntryData data, DailySummaryPanel summaryPanel);

    [SerializeField]
    private HomeViewTab _CozmoTabPrefab;

    [SerializeField]
    private HomeViewTab _PlayTabPrefab;

    [SerializeField]
    private HomeViewTab _ProfileTabPrefab;

    private HomeViewTab _CurrentTab;

    [SerializeField]
    private CozmoButton _CozmoTabButton;

    [SerializeField]
    private CozmoButton _PlayTabButton;

    [SerializeField]
    private CozmoButton _ProfileTabButton;

    [SerializeField]
    private RectTransform _ScrollRectContent;

    [SerializeField]
    DailySummaryPanel _DailySummaryPrefab;
    DailySummaryPanel _DailySummaryInstance;

    public delegate void ButtonClickedHandler(string challengeClicked, Transform buttonTransform);

    public event ButtonClickedHandler OnLockedChallengeClicked;
    public event ButtonClickedHandler OnUnlockedChallengeClicked;
    public event ButtonClickedHandler OnCompletedChallengeClicked;
    public event Action OnEndSessionClicked;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStates;

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById, Transform[] rewardIcons = null) {

      DASEventViewName = "home_view";

      _ChallengeStates = challengeStatesById;

      _CurrentTab = GameObject.Instantiate(_PlayTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);

      _CozmoTabButton.DASEventButtonName = "cozmo_tab_botton";
      _PlayTabButton.DASEventButtonName = "play_tab_button";
      _ProfileTabButton.DASEventButtonName = "profile_tab_button";

      _CozmoTabButton.DASEventViewController = "home_view";
      _PlayTabButton.DASEventViewController = "home_view";
      _ProfileTabButton.DASEventViewController = "home_view";

      _CozmoTabButton.onClick.AddListener(HandleCozmoTabButton);
      _PlayTabButton.onClick.AddListener(HandlePlayTabButton);
      _ProfileTabButton.onClick.AddListener(HandleProfileTabButton);

      UpdateDailySession(rewardIcons);

    }

    private void HandleCozmoTabButton() {
      if (_CurrentTab != null) {
        GameObject.Destroy(_CurrentTab.gameObject);
      }
      _CurrentTab = GameObject.Instantiate(_CozmoTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);
    }

    private void HandlePlayTabButton() {
      if (_CurrentTab != null) {
        GameObject.Destroy(_CurrentTab.gameObject);
      }
      _CurrentTab = GameObject.Instantiate(_PlayTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);
    }

    private void HandleProfileTabButton() {
      if (_CurrentTab != null) {
        GameObject.Destroy(_CurrentTab.gameObject);
      }
      _CurrentTab = GameObject.Instantiate(_ProfileTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);
    }

    private void UpdateDailySession(Transform[] rewardIcons = null) {
      var currentSession = DataPersistenceManager.Instance.CurrentSession;
      IRobot currentRobot = RobotEngineManager.Instance.CurrentRobot;
      // if current session is invalid then start a new session
      if (currentSession == null) {  
        var lastSession = DataPersistenceManager.Instance.Data.Sessions.LastOrDefault();

        if (lastSession != null && !lastSession.Complete) {
          CompleteSession(lastSession);
        }

        // start a new session
        TimelineEntryData newSession = new TimelineEntryData(DataPersistenceManager.Today) {
          StartingFriendshipLevel = RobotEngineManager.Instance.CurrentRobot.FriendshipLevel,
          StartingFriendshipPoints = RobotEngineManager.Instance.CurrentRobot.FriendshipPoints
        };

        StatContainer goals = DailyGoalManager.Instance.GenerateDailyGoals();
        newSession.Goals.Set(goals);

        currentRobot.SetProgressionStats(newSession.Progress);

        if (DailyGoalsSet != null) {
          DailyGoalsSet(newSession.Progress, newSession.Goals, rewardIcons);
        }

        DataPersistenceManager.Instance.Data.Sessions.Add(newSession);

        DataPersistenceManager.Instance.Save();
      }
    }

    private void CompleteSession(TimelineEntryData timelineEntry) {

      int friendshipPoints = DailyGoalManager.Instance.CalculateFriendshipPoints(timelineEntry.Progress, timelineEntry.Goals);

      RobotEngineManager.Instance.CurrentRobot.AddToFriendshipPoints(friendshipPoints);
      UpdateFriendshipPoints(timelineEntry, friendshipPoints);

      int stat_count = (int)Anki.Cozmo.ProgressionStatType.Count; 
      for (int i = 0; i < stat_count; ++i) {
        var targetStat = (Anki.Cozmo.ProgressionStatType)i;
        if (timelineEntry.Goals[targetStat] > 0) {
          DAS.Event(DASConstants.Goal.kProgressSummary, DASUtil.FormatDate(timelineEntry.Date), 
            new Dictionary<string,string> { {
                "$data",
                DASUtil.FormatGoal(targetStat, timelineEntry.Progress[targetStat], timelineEntry.Goals[targetStat])
              }
            });
        }
      }

      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.DailyGoal);

      ShowDailySessionPanel(timelineEntry, HandleOnFriendshipBarAnimateComplete);
    }

    private void ShowDailySessionPanel(TimelineEntryData session, OnFriendshipBarAnimateComplete onComplete = null) {
      if (_DailySummaryInstance != null) {
        return;
      }
      DailyGoalManager.Instance.DisableRequestGameBehaviorGroups();
      _DailySummaryInstance = UIManager.OpenView(_DailySummaryPrefab, 
        newView => {
          newView.Initialize(session);
        });
      if (onComplete != null) {
        _DailySummaryInstance.FriendshipBarAnimateComplete += onComplete;
      }
      _DailySummaryInstance.ViewClosed += HandleDailySummaryClosed;
    }

    private void HandleDailySummaryClosed() {
      DailyGoalManager.Instance.SetMinigameNeed();
    }

    private void HandleOnFriendshipBarAnimateComplete(TimelineEntryData data, DailySummaryPanel summaryPanel) {

      TimeSpan deltaTime = DataPersistenceManager.Instance.Data.Sessions.Count <= 1 ? new TimeSpan(1, 0, 0, 0) : 
        (DataPersistenceManager.Instance.Data.Sessions[DataPersistenceManager.Instance.Data.Sessions.Count - 2].Date - DataPersistenceManager.Today);
      int friendshipPoints = ((int)deltaTime.TotalDays + 1) * 10;
      summaryPanel.FriendshipBarAnimateComplete -= HandleOnFriendshipBarAnimateComplete;

      if (friendshipPoints < 0) {
        RobotEngineManager.Instance.CurrentRobot.AddToFriendshipPoints(friendshipPoints);
        UpdateFriendshipPoints(data, friendshipPoints);
        summaryPanel.AnimateFriendshipBar(data);
      }
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

    public Dictionary<string, ChallengeStatePacket> GetChallengeStates() {
      return _ChallengeStates;
    }

    public void HandleLockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnLockedChallengeClicked != null) {
        OnLockedChallengeClicked(challengeClicked, buttonTransform);
      } 
    }

    public void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnUnlockedChallengeClicked != null) {
        OnUnlockedChallengeClicked(challengeClicked, buttonTransform);
      } 
    }

    protected override void CleanUp() {
      
    }

  }
}
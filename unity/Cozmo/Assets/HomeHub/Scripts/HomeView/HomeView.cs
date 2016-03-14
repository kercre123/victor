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
    private HomeViewTab _CozmoTab;

    [SerializeField]
    private HomeViewTab _PlayTabPrefab;
    private HomeViewTab _PlayTab;

    [SerializeField]
    private HomeViewTab _ProfileTabPrefab;
    private HomeViewTab _ProfileTab;

    [SerializeField]
    private CozmoButton _CozmoTabButton;

    [SerializeField]
    private CozmoButton _PlayTabButton;

    [SerializeField]
    private CozmoButton _ProfileTabButton;

    [SerializeField]
    DailySummaryPanel _DailySummaryPrefab;
    DailySummaryPanel _DailySummaryInstance;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStates;

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById, Transform[] rewardIcons = null) {

      _ChallengeStates = challengeStatesById;

      _CozmoTab = UIManager.CreateUIElement(_CozmoTabPrefab.gameObject, this.transform).GetComponent<HomeViewTab>();
      _PlayTab = UIManager.CreateUIElement(_PlayTabPrefab.gameObject, this.transform).GetComponent<HomeViewTab>();
      _ProfileTab = UIManager.CreateUIElement(_ProfileTabPrefab.gameObject, this.transform).GetComponent<HomeViewTab>();

      _CozmoTab.Initialize(this);
      _PlayTab.Initialize(this);
      _ProfileTab.Initialize(this);

      _CozmoTabButton.onClick.AddListener(HandleCozmoTabButton);
      _PlayTabButton.onClick.AddListener(HandlePlayTabButton);
      _ProfileTabButton.onClick.AddListener(HandleProfileTabButton);

      UpdateDailySession(rewardIcons);

    }

    private void HandleCozmoTabButton() {
      
    }

    private void HandlePlayTabButton() {
      
    }

    private void HandleProfileTabButton() {
      
    }

    private void UpdateDailySession(Transform[] rewardIcons = null) {
      var currentSession = DataPersistenceManager.Instance.CurrentSession;
      IRobot currentRobot = RobotEngineManager.Instance.CurrentRobot;
      // check if the current session is still valid
      if (currentSession != null && DailyGoalsSet != null) {  
        DailyGoalsSet(currentSession.Progress, currentSession.Goals, rewardIcons);
      }
      else {
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

    protected override void CleanUp() {
      
    }

  }
}
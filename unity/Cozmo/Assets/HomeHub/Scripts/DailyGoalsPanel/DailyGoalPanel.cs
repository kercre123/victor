using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using System.Linq;
using System;
using Anki.Cozmo;
using DataPersistence;

/// <summary>
/// Daily goal panel. Displays list of tasks and progress for the day.
/// </summary>
public class DailyGoalPanel : MonoBehaviour {

  private readonly List<GoalCell> _GoalCells = new List<GoalCell>();
  private const float _kFadeTweenDuration = 0.25f;

  public delegate void OnFriendshipBarAnimateComplete(TimelineEntryData data, DailySummaryPanel summaryPanel);

  // Prefab for GoalCells
  [SerializeField]
  private GoalCell _GoalCellPrefab;

  // Container for Daily Goal Cells to be children of
  [SerializeField]
  private Transform _GoalContainer;


  [SerializeField]
  private RectTransform _Title;

  [SerializeField]
  private AnimationCurve _TitleScaleCurve;

  [SerializeField]
  private DailySummaryPanel _DailySummaryPrefab;
  private DailySummaryPanel _DailySummaryInstance;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _CompletedText;

  void Start() {
    UpdateDailySession(GetComponent<Cozmo.HomeHub.TabPanel>().GetHomeViewInstance().HomeHubInstance.RewardIcons);
  }

  private void OnDestroy() {
    _GoalCells.Clear();
  }

  public void UpdateDailySession(Transform[] rewardIcons = null) {
    var currentSession = DataPersistenceManager.Instance.CurrentSession;

    if (currentSession != null) {

      SetDailyGoals(currentSession.DailyGoals);

      if (currentSession.GoalsFinished == false &&
          DailyGoalManager.Instance.AreAllDailyGoalsComplete()) {
        currentSession.GoalsFinished = true;
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.DailyGoal);
      }
    }
    else {
      var lastSession = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault();

      if (lastSession != null && !lastSession.Complete) {
        CompleteSession(lastSession);
      }

      TimelineEntryData newSession = new TimelineEntryData(DataPersistenceManager.Today);

      newSession.DailyGoals = DailyGoalManager.Instance.GenerateDailyGoals();

      SetDailyGoals(newSession.DailyGoals);

      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Add(newSession);

      DataPersistenceManager.Instance.Save();
    }

    rewardIcons = null;
  }

  private void OnGoalCellProgression(GoalCell goalCell) {
    UpdateCompletedText();
  }

  private int GetGoalsCompletedCount() {
    int completed = 0;
    for (int i = 0; i < _GoalCells.Count; ++i) {
      if (_GoalCells[i].GoalComplete()) {
        completed++;
      }
    }
    return completed;
  }

  private void CompleteSession(TimelineEntryData timelineEntry) {
    
    if (timelineEntry.DailyGoals.Count > 0) {
      for (int i = 0; i < timelineEntry.DailyGoals.Count; i++) {
        DAS.Event(DASConstants.Goal.kProgressSummary, DASUtil.FormatDate(timelineEntry.Date), 
          new Dictionary<string,string> { {
              "$data",
              DASUtil.FormatGoal(timelineEntry.DailyGoals[i])
            }
          });
      }
    }

    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.DailyGoal);

    ShowDailySessionPanel(timelineEntry);
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

  // TODO: Flesh this out if necessary, do we want to salvage the rewardIcons?
  public void SetDailyGoals(List<DailyGoal> dailyGoals) {
    for (int i = 0; i < dailyGoals.Count; i++) {
      CreateGoalCell(dailyGoals[i]);
    }
    UpdateCompletedText();
    DailyGoalManager.Instance.SetMinigameNeed();
  }


  private void UpdateCompletedText() {
    _CompletedText.FormattingArgs = new object[] { GetGoalsCompletedCount(), _GoalCells.Count };
  }

  // Creates a goal badge based on a specified DailyGoal, then hooks in to DailyGoalManager.
  private GoalCell CreateGoalCell(DailyGoal goal) {
    DAS.Event(this, string.Format("CreateGoalCell({0})", goal.Title));
    GoalCell newBadge = UIManager.CreateUIElement(_GoalCellPrefab.gameObject, _GoalContainer).GetComponent<GoalCell>();
    newBadge.Initialize(goal);
    newBadge.OnProgChanged += OnGoalCellProgression;
    _GoalCells.Add(newBadge);
    return newBadge;
  }

}

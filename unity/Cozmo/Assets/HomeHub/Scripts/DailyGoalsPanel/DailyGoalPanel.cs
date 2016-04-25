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

  public delegate void OnFriendshipBarAnimateComplete(TimelineEntryData data,DailySummaryPanel summaryPanel);

  // Prefab for GoalCells
  [SerializeField]
  private GoalCell _GoalCellPrefab;

  [SerializeField]
  private Transform _BonusBarContainer;
  [SerializeField]
  private BonusBarPanel _BonusBarPrefab;
  private BonusBarPanel _BonusBarPanel;

  // Progress bar for tracking total progress for all Goals
  [SerializeField]
  private ProgressBar _TotalProgressBar;

  // Container for Daily Goal Cells to be children of
  [SerializeField]
  private Transform _GoalContainer;


  [SerializeField]
  private RectTransform _Title;
  [SerializeField]
  private RectTransform _TitleGlow;

  [SerializeField]
  private AnimationCurve _TitleScaleCurve;


  private RectTransform _RectTransform;

  private bool _Expanded = true;

  [SerializeField]
  private DailySummaryPanel _DailySummaryPrefab;
  private DailySummaryPanel _DailySummaryInstance;

  void Awake() {
    _RectTransform = GetComponent<RectTransform>();
    _BonusBarPanel = UIManager.CreateUIElement(_BonusBarPrefab.gameObject, _BonusBarContainer).GetComponent<BonusBarPanel>();
    //TODO: Refactor BonusBarPanel based on new design or remove entirely
    _BonusBarPanel.gameObject.SetActive(false);
  }

  void Start() {
    UpdateDailySession(GetComponent<Cozmo.HomeHub.TabPanel>().GetHomeViewInstance().HomeHubInstance.RewardIcons);
  }

  private void OnDestroy() {
    for (int i = 0; i < _GoalCells.Count; i++) {
      _GoalCells[i].OnProgChanged -= RefreshProgress;
    }
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

  private void UpdateFriendshipPoints(TimelineEntryData timelineEntry, int friendshipPoints) {

  }

  private void HandleDailySummaryClosed() {
    DailyGoalManager.Instance.SetMinigameNeed();
  }

  // TODO: Flesh this out if necessary, do we want to salvage the rewardIcons?
  public void SetDailyGoals(List<DailyGoal> dailyGoals) {
    for (int i = 0; i < dailyGoals.Count; i++) {
      CreateGoalCell(dailyGoals[i]);
    }
    _TotalProgressBar.SetProgress(DailyGoalManager.Instance.GetTodayProgress());
    DailyGoalManager.Instance.SetMinigameNeed();
  }

  // Creates a goal badge based on a specified DailyGoal, then hooks in to DailyGoalManager.
  private GoalCell CreateGoalCell(DailyGoal goal) {
    DAS.Event(this, string.Format("CreateGoalCell({0})", goal.Title));
    GoalCell newBadge = UIManager.CreateUIElement(_GoalCellPrefab.gameObject, _GoalContainer).GetComponent<GoalCell>();
    newBadge.Initialize(goal);
    _GoalCells.Add(newBadge);
    newBadge.OnProgChanged += RefreshProgress;
    return newBadge;
  }

  private void RefreshProgress() {
    _TotalProgressBar.SetProgress(DataPersistence.DataPersistenceManager.Instance.CurrentSession.GetTotalProgress());
  }

  // Show Hidden UI Elements when Expanding back to full
  public void Expand() {
    if (!_Expanded) {
      Sequence fadeTween = DOTween.Sequence();
      for (int i = 0; i < _GoalCells.Count; i++) {
        GoalCell cell = _GoalCells[i];
        fadeTween.Join(cell.GoalLabel.DOFade(1.0f, _kFadeTweenDuration));
      }
      fadeTween.Join(_BonusBarPanel.BonusBarCanvas.DOFade(1.0f, _kFadeTweenDuration));
      fadeTween.Play();
      _Expanded = true;
    }
  }

  // Fade out Hidden UI elements when collapsed
  public void Collapse() {
    if (_Expanded) {
      Sequence fadeTween = DOTween.Sequence();
      for (int i = 0; i < _GoalCells.Count; i++) {
        GoalCell cell = _GoalCells[i];
        fadeTween.Join(cell.GoalLabel.DOFade(0.0f, _kFadeTweenDuration));
      }
      fadeTween.Join(_BonusBarPanel.BonusBarCanvas.DOFade(0.0f, _kFadeTweenDuration));
      fadeTween.Play();
      _Expanded = false;
    }
  }

  private void Update() {
    var rect = _RectTransform.rect;

    _TitleGlow.localScale = _Title.localScale = Vector3.one * _TitleScaleCurve.Evaluate(rect.width);
  }

}

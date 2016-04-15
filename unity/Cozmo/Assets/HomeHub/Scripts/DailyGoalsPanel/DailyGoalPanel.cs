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

// Panel for generating and displaying the ProgressionStat goals for the Day.
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

  private GoalCell[] _GoalCellsByStat;
  private readonly List<Sequence> _RewardTweens = new List<Sequence>();

  [SerializeField]
  private DailySummaryPanel _DailySummaryPrefab;
  private DailySummaryPanel _DailySummaryInstance;

  void Awake() {
    _RectTransform = GetComponent<RectTransform>();
    _BonusBarPanel = UIManager.CreateUIElement(_BonusBarPrefab.gameObject, _BonusBarContainer).GetComponent<BonusBarPanel>();
    _GoalCellsByStat = new GoalCell[(int)ProgressionStatType.Count];
  }

  void Start() {
    UpdateDailySession(GetComponent<Cozmo.HomeHub.TabPanel>().GetHomeViewInstance().HomeHubInstance.RewardIcons);
  }

  private void OnDestroy() {
    for (int i = 0; i < _GoalCells.Count; i++) {
      _GoalCells[i].OnProgChanged -= RefreshProgress;
    }
    _GoalCells.Clear();

    if (_RewardTweens != null) {
      StopCoroutine(DelayedAnimateRewards(null));
      foreach (var tween in _RewardTweens) {
        tween.Kill();
      }
      _RewardTweens.Clear();
    }
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
      // TODO: Generate new Daily Goals and set them properly here
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

  // TODO: Kill this. Create DailyGoals based on DailyGoalList. Do we even want reward Icons?
  /*
  public void SetDailyGoals(StatContainer progress, StatContainer goals, Transform[] rewardIcons = null) {
    for (int i = 0; i < (int)ProgressionStatType.Count; i++) {
      var targetStat = (ProgressionStatType)i;
      if (goals[targetStat] > 0) {
        CreateGoalCell(targetStat, progress[targetStat], goals[targetStat]);
      }
    }
    float dailyProg = DailyGoalManager.Instance.CalculateDailyGoalProgress(progress, goals);
    float bonusMult = DailyGoalManager.Instance.CalculateBonusMult(progress, goals);
    _TotalProgressBar.SetProgress(dailyProg);
    _BonusBarPanel.SetFriendshipBonus(bonusMult);

    DailyGoalManager.Instance.SetMinigameNeed();

    if (rewardIcons != null) {
      bool anyExists = false; 
      foreach (var reward in rewardIcons) {
        if (reward != null) {
          anyExists = true;
          break;
        }
      }
      if (anyExists) {
        StartCoroutine(DelayedAnimateRewards(rewardIcons));
      }
    }
  }*/

  // TODO: Flesh this out if necessary, do we want to salvage the rewardIcons?
  public void SetDailyGoals(List<DailyGoal> dailyGoals) {
    for (int i = 0; i < dailyGoals.Count; i++) {
      CreateGoalCell(dailyGoals[i]);
    }
    _TotalProgressBar.SetProgress(DailyGoalManager.Instance.GetTodayProgress());
    DailyGoalManager.Instance.SetMinigameNeed();
  }

  // TODO: Use this instead. Creates a goal badge based on a specified DailyGoal, then hooks in to DailyGoalManager.
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

  private IEnumerator DelayedAnimateRewards(Transform[] rewardIconsByStat) {
    // Wait so that the goal cells can lay out
    yield return new WaitForSeconds(0.1f);

    // TODO: For each reward, tween it to the target goal
    _RewardTweens.Clear();

    for (int stat = 0; stat < (int)ProgressionStatType.Count; stat++) {
      if (rewardIconsByStat[stat] != null) {
        if (_GoalCellsByStat[stat] != null) {
          CreateSequenceForRewardIcon(rewardIconsByStat[stat], _GoalCellsByStat[stat].transform);
        }
        else {
          DAS.Error(this, string.Format("Could not find GoalCell for stat {0} when tweening Rewards!", 
            (ProgressionStatType)stat));
        }
      }
    }
    if (rewardIconsByStat.Length > 0) {
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.RelationshipGain);
    }
  }

  private void CreateSequenceForRewardIcon(Transform target, Transform goalCell) {
    var rewardTweenSequence = DOTween.Sequence();
    var rewardIconTween = target.DOMove(goalCell.position, 1.5f)
      .SetDelay(_RewardTweens.Count * 0.1f).SetEase(Ease.InCubic).OnComplete(() => {
      Destroy(target.gameObject);
    });
    rewardTweenSequence.Join(rewardIconTween);
    rewardTweenSequence.Play();
    _RewardTweens.Add(rewardTweenSequence);
  }
}

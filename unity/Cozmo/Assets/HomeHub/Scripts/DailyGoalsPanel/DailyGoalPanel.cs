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
  private readonly List<GameObject> _EmptyGoalCells = new List<GameObject>();
  private const float _kFadeTweenDuration = 0.25f;

  public delegate void OnFriendshipBarAnimateComplete(TimelineEntryData data,DailySummaryPanel summaryPanel);

  // Prefab for GoalCells
  [SerializeField]
  private GoalCell _GoalCellPrefab;

  [SerializeField]
  private GameObject _EmptyGoalCellPrefab;

  // Container for Daily Goal Cells to be children of
  [SerializeField]
  private Transform _GoalContainer;

  [SerializeField]
  private RectTransform _Title;

  [SerializeField]
  private AnimationCurve _TitleScaleCurve;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _CompletedText;

  void Start() {
    UpdateDailySession();
  }

  private void OnDestroy() {
    ClearGoalCells();
  }

  private void ClearGoalCells() {
    foreach (GoalCell cell in _GoalCells) {
      if (cell != null) {
        GameObject.Destroy(cell.gameObject);
      }
    }
    _GoalCells.Clear();
    foreach (GameObject cell in _EmptyGoalCells) {
      GameObject.Destroy(cell);
    }
    _EmptyGoalCells.Clear();
  }

  public void UpdateDailySession() {
    var currentSession = DataPersistenceManager.Instance.CurrentSession;
    if (currentSession == null) {
      currentSession = DataPersistenceManager.Instance.StartNewSession();
    }

    SetDailyGoals(currentSession.DailyGoals);

    if (currentSession.GoalsFinished == false &&
        DailyGoalManager.Instance.AreAllDailyGoalsComplete()) {
      currentSession.GoalsFinished = true;
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.DailyGoal);
    }
    // TODO: UpdateDailySession should reflect rewards earned during that session rather than passing the rewardIcons through a spaghetti chain
    // RewardedAction and UnlockManagers should save that data to the session.
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
          new Dictionary<string, string> { {
              "$data",
              DASUtil.FormatGoal(timelineEntry.DailyGoals[i])
            }
          });
      }
    }

    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.DailyGoal);
  }


  public void SetDailyGoals(List<DailyGoal> dailyGoals) {
    for (int i = 0; i < dailyGoals.Count; i++) {
      GoalCell cell = CreateGoalCell(dailyGoals[i]);
      if (i == DailyGoalManager.Instance.GetConfigMaxGoalCount() - 1 && cell.GetComponent<GoalCellHorizontalBar>() != null) {
        cell.GetComponent<GoalCellHorizontalBar>().SetHorizontalMarker(false);
      }
    }

    // putting in empty slots for empty goals.
    for (int i = dailyGoals.Count; i < DailyGoalManager.Instance.GetConfigMaxGoalCount(); ++i) {
      GameObject cell = CreateEmptyGoalCell();
      if (i == DailyGoalManager.Instance.GetConfigMaxGoalCount() - 1 && cell.GetComponent<GoalCellHorizontalBar>() != null) {
        cell.GetComponent<GoalCellHorizontalBar>().SetHorizontalMarker(false);
      }
    }

    UpdateCompletedText();
  }


  private void UpdateCompletedText() {
    _CompletedText.FormattingArgs = new object[] { GetGoalsCompletedCount(), _GoalCells.Count };
  }

  // Creates a goal badge based on a specified DailyGoal, then hooks in to DailyGoalManager.
  private GoalCell CreateGoalCell(DailyGoal goal) {
    DAS.Info("DailyGoalPanel.CreateGoalCell", string.Format("CreateGoalCell({0})", goal.Title));
    GoalCell newBadge = UIManager.CreateUIElement(_GoalCellPrefab.gameObject, _GoalContainer).GetComponent<GoalCell>();
    newBadge.Initialize(goal);
    newBadge.OnProgChanged += OnGoalCellProgression;
    _GoalCells.Add(newBadge);
    return newBadge;
  }

  private GameObject CreateEmptyGoalCell() {
    GameObject newEmptyBadge = (UIManager.CreateUIElement(_EmptyGoalCellPrefab, _GoalContainer));
    _EmptyGoalCells.Add(newEmptyBadge);
    return newEmptyBadge;
  }

}

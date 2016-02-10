using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;

// Panel for generating and displaying the ProgressionStat goals for the Day.
public class DailyGoalPanel : MonoBehaviour {
  
  private readonly List<GoalCell> _GoalCells = new List<GoalCell>();
  private const float _kFadeTweenDuration = 0.25f;


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

  void Awake() {
    _RectTransform = GetComponent<RectTransform>();
    _BonusBarPanel = UIManager.CreateUIElement(_BonusBarPrefab.gameObject, _BonusBarContainer).GetComponent<BonusBarPanel>();
  }

  public void SetDailyGoals(StatContainer progress, StatContainer goals) {
    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; i++) {
      var targetStat = (Anki.Cozmo.ProgressionStatType)i;
      if (goals[targetStat] > 0) {
        CreateGoalCell(targetStat, progress[targetStat], goals[targetStat]);
      }
    }
    float dailyProg = DailyGoalManager.Instance.CalculateDailyGoalProgress(progress, goals);
    float bonusMult = DailyGoalManager.Instance.CalculateBonusMult(progress, goals);
    _TotalProgressBar.SetProgress(dailyProg);
    _BonusBarPanel.SetFriendshipBonus(bonusMult);

    float currNeed = DailyGoalManager.Instance.GetMinigameNeed_Extremes();
    RobotEngineManager.Instance.CurrentRobot.AddToEmotion(Anki.Cozmo.EmotionType.WantToPlay, currNeed, "DailyGoalProgress");
    DailyGoalManager.Instance.PickMiniGameToRequest();
  }

  // Creates a goal badge based on a progression stat and adds to the DailyGoal in RobotEngineManager
  public GoalCell CreateGoalCell(Anki.Cozmo.ProgressionStatType type, int prog, int goal) {
    DAS.Event(this, string.Format("CreateGoalCell({0},{1},{2})", type, prog, goal));
    GoalCell newBadge = UIManager.CreateUIElement(_GoalCellPrefab.gameObject, _GoalContainer).GetComponent<GoalCell>();
    newBadge.Initialize(type, prog, goal);
    _GoalCells.Add(newBadge);
    newBadge.OnProgChanged += RefreshProgress;
    return newBadge;
  }

  public void RefreshProgress() {
    StatContainer progress = DataPersistence.DataPersistenceManager.Instance.CurrentSession.Progress;
    StatContainer goals = DataPersistence.DataPersistenceManager.Instance.CurrentSession.Goals;
    float dailyProg = DailyGoalManager.Instance.CalculateDailyGoalProgress(progress, goals);
    float bonusMult = DailyGoalManager.Instance.CalculateBonusMult(progress, goals);
    _TotalProgressBar.SetProgress(dailyProg);
    _BonusBarPanel.SetFriendshipBonus(bonusMult);
  }

  // Show Hidden UI Elements when Expanding back to full
  public void Expand() {
    Sequence fadeTween = DOTween.Sequence();
    for (int i = 0; i < _GoalCells.Count; i++) {
      GoalCell cell = _GoalCells[i];
      fadeTween.Join(cell.GoalLabel.DOFade(1.0f, _kFadeTweenDuration));
    }
    fadeTween.Join(_BonusBarPanel.BonusBarCanvas.DOFade(1.0f, _kFadeTweenDuration));
    fadeTween.Play();
  }

  // Fade out Hidden UI elements when collapsed
  public void Collapse() {
    Sequence fadeTween = DOTween.Sequence();
    for (int i = 0; i < _GoalCells.Count; i++) {
      GoalCell cell = _GoalCells[i];
      fadeTween.Join(cell.GoalLabel.DOFade(0.0f, _kFadeTweenDuration));
    }
    fadeTween.Join(_BonusBarPanel.BonusBarCanvas.DOFade(0.0f, _kFadeTweenDuration));
    fadeTween.Play();
  }

  private void Update() {
    var rect = _RectTransform.rect;

    _TitleGlow.localScale = _Title.localScale = Vector3.one * _TitleScaleCurve.Evaluate(rect.width);
  }

  private void OnDestroy() {
    for (int i = 0; i < _GoalCells.Count; i++) {
      _GoalCells[i].OnProgChanged -= RefreshProgress;
    }
    _GoalCells.Clear();
  }
}

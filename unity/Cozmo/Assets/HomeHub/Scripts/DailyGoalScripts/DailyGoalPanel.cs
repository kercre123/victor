using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;

// ProgressionStatType
using Anki.Cozmo;


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

  private GoalCell[] _GoalCellsByStat;
  private List<Sequence> _RewardTweens;

  void Awake() {
    _RectTransform = GetComponent<RectTransform>();
    _BonusBarPanel = UIManager.CreateUIElement(_BonusBarPrefab.gameObject, _BonusBarContainer).GetComponent<BonusBarPanel>();
    _GoalCellsByStat = new GoalCell[(int)ProgressionStatType.Count];
  }

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

    if (dailyProg > 0) {
      // This might not be the best place to do this, but it should be ok for now.
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.RelationshipGain);
    }

    float currNeed = DailyGoalManager.Instance.GetMinigameNeed_Extremes();
    RobotEngineManager.Instance.CurrentRobot.AddToEmotion(EmotionType.WantToPlay, currNeed, "DailyGoalProgress");
    DailyGoalManager.Instance.PickMiniGameToRequest();

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
  }

  // Creates a goal badge based on a progression stat and adds to the DailyGoal in RobotEngineManager
  private GoalCell CreateGoalCell(ProgressionStatType type, int prog, int goal) {
    DAS.Event(this, string.Format("CreateGoalCell({0},{1},{2})", type, prog, goal));
    GoalCell newBadge = UIManager.CreateUIElement(_GoalCellPrefab.gameObject, _GoalContainer).GetComponent<GoalCell>();
    newBadge.Initialize(type, prog, goal);
    _GoalCells.Add(newBadge);
    _GoalCellsByStat[(int)type] = newBadge;
    newBadge.OnProgChanged += RefreshProgress;
    return newBadge;
  }

  private void RefreshProgress() {
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

    if (_RewardTweens != null) {
      StopCoroutine(DelayedAnimateRewards(null));
      foreach (var tween in _RewardTweens) {
        tween.Kill();
      }
      _RewardTweens.Clear();
    }
  }

  private IEnumerator DelayedAnimateRewards(Transform[] rewardIconsByStat) {
    // Wait so that the goal cells can lay out
    yield return new WaitForSeconds(0.1f);

    // TODO: For each reward, tween it to the target goal
    _RewardTweens = new List<Sequence>();
    Sequence rewardTweenSequence;
    Tweener rewardIconTween;
    Transform target;
    for (int stat = 0; stat < (int)ProgressionStatType.Count; stat++) {
      if (rewardIconsByStat[stat] != null) {
        if (_GoalCellsByStat[stat] != null) {
          rewardTweenSequence = DOTween.Sequence();
          target = rewardIconsByStat[stat];
          rewardIconTween = rewardIconsByStat[stat].DOMove(_GoalCellsByStat[stat].transform.position, 1.5f)
            .SetDelay(_RewardTweens.Count * 0.1f).SetEase(Ease.InOutBack).OnComplete(() => {
            Destroy(target.gameObject);
          });
          rewardTweenSequence.Join(rewardIconTween);
          rewardTweenSequence.Play();
          _RewardTweens.Add(rewardTweenSequence);
        }
        else {
          DAS.Error(this, string.Format("Could not find GoalCell for stat {0} when tweening Rewards!", 
            (ProgressionStatType)stat));
        }
      }
    }
  }
}

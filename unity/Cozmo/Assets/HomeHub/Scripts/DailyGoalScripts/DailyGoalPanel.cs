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


  // Prefab for GoalCells
  [SerializeField]
  private GoalCell _GoalCellPrefab;

  [SerializeField]
  private BonusBarPanel _BonusBarPanel;

  // Progress bar for tracking total progress for all Goals
  [SerializeField]
  private ProgressBar _TotalProgressBar;

  // Container for Daily Goal Cells to be children of
  [SerializeField]
  private Transform _GoalContainer;

  private void OnDestroy() {
    _GoalCells.Clear();
  }

  public void SetDailyGoals(StatContainer progress, StatContainer goals) {
    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; i++) {
      var targetStat = (Anki.Cozmo.ProgressionStatType)i;
      if (goals[targetStat] > 0) {
        CreateGoalCell(targetStat, goals[targetStat], progress[targetStat]);
      }
    }
    float dailyProg = DailyGoalManager.Instance.GetFriendForumulaConfig().CalculateDailyGoalProgress(progress, goals);
    _TotalProgressBar.SetProgress(dailyProg);
    _BonusBarPanel.ToggleFriendshipBonus(dailyProg >= 1.0f, dailyProg);
  }

  // Creates a goal badge based on a progression stat and adds to the DailyGoal in RobotEngineManager
  // Currently this will be additive so if multiple Goals are created with the same required type, they will be combined
  public GoalCell CreateGoalCell(Anki.Cozmo.ProgressionStatType type, int prog, int goal) {
    DAS.Event(this, string.Format("CreateGoalCell({0},{1})", type, prog));
    GoalCell newBadge = UIManager.CreateUIElement(_GoalCellPrefab.gameObject, _GoalContainer).GetComponent<GoalCell>();
    newBadge.Initialize(type, prog, goal);
    _GoalCells.Add(newBadge);
    return newBadge;
  }

  // Disable any UI elements that should not be shown when collapsed
  public void Collapse(bool collapse) {
    _BonusBarPanel.gameObject.SetActive(collapse);
  }
}

using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Cozmo.UI;

/// <summary>
/// Full List of Pairs for CladEvents to AnimationGroups.
/// </summary>
namespace Anki.Cozmo {
  public class DailyGoalGenerationData {

    // Comparer to properly sort Daily Goal Gen by the GoalEvents
    public class GoalDataComparer : IComparer<GoalEntry> {
      public int Compare(GoalEntry x, GoalEntry y) {
        if (x == null) {
          if (y == null) {
            // If both null, equals
            return 0;
          }
          else {
            // If x null and y isn't, y is greater
            return -1;
          }
        }
        else {
          // If y null, x is greater
          if (y == null) {
            return 1;
          }
          else {
            // If both aren't null, actually compare CladEvent values
            return ((int)x.CladEvent).CompareTo((int)y.CladEvent);
          }
        }
      }
    }

    public List<GoalEntry> GenList = new List<GoalEntry>();

    // TODO: Set up all necessary DailyGoal info, as well as additional info for determining generation
    [System.Serializable]
    public class GoalEntry {
      public GoalEntry() {
        TitleKey = "";
        DescKey = "";
        Target = 1;
        PointsRewarded = 0;
        RewardType = "experience";
        CladEvent = GameEvent.Count;
        GenConditions = new List<GoalCondition>();
      }
      // TODO: Preconditions for limiting generation
      // TODO: Everything to do with the goal, implement
      public string TitleKey;
      public string DescKey;
      public string RewardType;
      public int Target;
      public int PointsRewarded;
      //
      public GameEvent CladEvent;
      public List<GoalCondition> GenConditions;

      public bool CanGen() {
        for (int i = 0; i < GenConditions.Count; i++) {
          if (GenConditions[i].ConditionMet() == false) {
            return false;
          }
        }
        return true;
      }
    }
  }
}


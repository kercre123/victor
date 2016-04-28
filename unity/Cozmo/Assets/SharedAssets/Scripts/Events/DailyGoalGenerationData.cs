using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Cozmo;
using Cozmo.UI;
using Cozmo.Util;

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

    [System.Serializable]
    public class GoalEntry {
      public GoalEntry() {
        TitleKey = "dailyGoal.title.missing";
        DescKey = "dailyGoal.description.missing";
        Target = 1;
        PointsRewarded = 0;
        RewardType = "experience";
        CladEvent = GameEvent.Count;
        GenConditions = new List<GoalCondition>();
        ProgressConditions = new List<GoalCondition>();
      }

      /// <summary>
      /// The title key for localization.
      /// </summary>
      public string TitleKey;
      /// <summary>
      /// The description key for localization.
      /// </summary>
      public string DescKey;
      /// <summary>
      /// The Item type of the reward.
      /// </summary>
      [ItemId]
      public string RewardType;
      /// <summary>
      /// The amount of the RewardType rewarded.
      /// </summary>
      public int PointsRewarded;
      /// <summary>
      /// The Target amount of times the CladEvent must be fired to complete this goal.
      /// </summary>
      public int Target;
      /// <summary>
      /// The Game Event that this Daily Goal is progressed by.
      /// </summary>
      public GameEvent CladEvent;
      /// <summary>
      /// The Conditions for if this goal is able to be selected for generation.
      /// </summary>
      public List<GoalCondition> GenConditions;
      /// <summary>
      /// The Conditions for if this goal will progress when its event is fired.
      /// </summary>
      public List<GoalCondition> ProgressConditions;

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


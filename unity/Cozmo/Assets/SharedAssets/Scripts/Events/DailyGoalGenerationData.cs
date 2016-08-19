using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Cozmo;
using Cozmo.UI;
using Cozmo.Util;
using Newtonsoft.Json;

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
        Target = 1;
        PointsRewarded = 0;
        RewardType = "experience";
        Tag = "";
        CladEvent = GameEvent.Count;
        GenConditions = new List<GoalCondition>();
        ProgressConditions = new List<GoalCondition>();
        Priority = 0;
        Id = 0;
      }

      /// <summary>
      /// The title key for localization.
      /// </summary>
      public string TitleKey;
      /// <summary>
      /// The Item type of the reward.
      /// </summary>
      [ItemId]
      public string RewardType;
      /// <summary>
      /// The tag for generation
      /// </summary>
      public string Tag;
      /// <summary>
      /// The amount of the RewardType rewarded.
      /// </summary>
      public int PointsRewarded;
      /// <summary>
      /// The Target amount of times the CladEvent must be fired to complete this goal.
      /// </summary>
      public int Target;
      /// <summary>
      /// Higher priority goals will sort to the top of the daily goal list.
      /// </summary>
      public int Priority;
      /// <summary>
      /// The Game Event that this Daily Goal is progressed by.
      /// </summary>
      public GameEvent CladEvent;
      /// <summary>
      /// The Conditions for if this goal is able to be selected for generation.
      /// </summary>
      public List<GoalCondition> GenConditions = new List<GoalCondition>();
      /// <summary>
      /// The Conditions for if this goal will progress when its event is fired.
      /// </summary>
      public List<GoalCondition> ProgressConditions = new List<GoalCondition>();
      /// <summary>
      /// Uint ID of the Goal, purely used in editor.
      /// </summary>
      public uint Id;

      public bool CanGen() {
        for (int i = 0; i < GenConditions.Count; i++) {
          if (GenConditions[i].ConditionMet() == false) {
            return false;
          }
        }
        return true;
      }

      /// <summary>
      /// Creates a copy of this instance and returns it with an Id of 0
      /// </summary>
      public GoalEntry Copy() {
        GoalEntry newGoal;
        string json = "";
        json = JsonConvert.SerializeObject(this, Formatting.Indented, new JsonSerializerSettings {
          TypeNameHandling = TypeNameHandling.Auto
        });
        newGoal = JsonConvert.DeserializeObject<GoalEntry>(json, new JsonSerializerSettings {
          TypeNameHandling = TypeNameHandling.Auto
        });
        newGoal.Id = 0;
        return newGoal;
      }
    }
  }
}


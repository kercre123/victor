using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Goal condition, checked during generation to determine whether or not we can select a daily goal from the pool.
/// TODO: Write JSON converter for Goal Conditions.
/// </summary>
namespace Cozmo {
  namespace UI {
    [System.Serializable]
    public abstract class GoalCondition {
      // TODO: Abstract class for Conditions
      [SerializeField]
      private string _Name;

      // Any extra parameters that are valid for specific conditions
      public Dictionary<string, object> CustomParams = new Dictionary<string, object>();

      // Override with unique checks to see if certain conditions are met by the current game state
      public virtual bool ConditionMet() {
        return true;
      }
    }
  }
}

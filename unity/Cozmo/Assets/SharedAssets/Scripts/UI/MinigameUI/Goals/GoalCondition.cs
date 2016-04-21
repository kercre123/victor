using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Goal condition, checked during generation to determine whether or not we can select a daily goal from the pool.
/// TODO: Write JSON converter for Goal Conditions.
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public abstract class GoalCondition {

      public virtual void Initialize() {
      }

      // Override with unique checks to see if certain conditions are met by the current game state
      public virtual bool ConditionMet() {
        return true;
      }
    }
  }
}

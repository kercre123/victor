using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Goal condition that specifies a day of the week that must be met.
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class WeekdayCondition : GoalCondition {
      
      DayOfWeek day = DayOfWeek.Monday;

      public override void Initialize() {
        base.Initialize();
      }

      // Override with unique checks to see if certain conditions are met by the current game state
      public override bool ConditionMet() {
        // TODO : Returns true if day of the week is the desired day of the week
        return (DateTime.UtcNow.DayOfWeek == day);
      }
    }
  }
}

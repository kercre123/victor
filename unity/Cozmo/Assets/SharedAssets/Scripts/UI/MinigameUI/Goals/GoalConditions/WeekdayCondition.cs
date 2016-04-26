using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;


#if UNITY_EDITOR
using UnityEditor;
#endif
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

      // Returns true if day of the week is the desired day of the week
      public override bool ConditionMet() {
        return (DateTime.UtcNow.DayOfWeek == day);
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        day = (DayOfWeek)EditorGUILayout.EnumPopup("Day", day);
      }
      #endif
    }
  }
}

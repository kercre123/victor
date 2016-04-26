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
      [SerializeField]
      public string dayName;
      DayOfWeek day;

      // Returns true if day of the week is the desired day of the week
      public override bool ConditionMet() {
        return (DateTime.UtcNow.DayOfWeek.ToString() == dayName);
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        if (day.ToString() != dayName) {
          if (!strToDay.TryGetValue(dayName, out day)) {
            day = DayOfWeek.Sunday;
          }
        }
        day = (DayOfWeek)EditorGUILayout.EnumPopup("Day", day);
        dayName = day.ToString();
      }
      // Map strings to DayOfWeek because DayOfWeek is the worst... THE WORST
      Dictionary<string, DayOfWeek> strToDay = new Dictionary<string, DayOfWeek>() {
        { "Monday", DayOfWeek.Monday },
        { "Tuesday", DayOfWeek.Tuesday },
        { "Wednesday", DayOfWeek.Wednesday },
        { "Thursday", DayOfWeek.Thursday },
        { "Friday", DayOfWeek.Friday },
        { "Saturday", DayOfWeek.Saturday },
        { "Sunday", DayOfWeek.Sunday }
      };
      #endif
    }
  }
}

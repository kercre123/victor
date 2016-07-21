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
    public class CurrentWeekdayCondition : GoalCondition {
      
      [SerializeField]
      private string _DayName;
      private DayOfWeek _Day;

      // Returns true if day of the week is the desired day of the week
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        return (DateTime.UtcNow.DayOfWeek.ToString() == _DayName);
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        if (_Day.ToString() != _DayName) {
          try {
            _Day = (DayOfWeek)Enum.Parse(typeof(DayOfWeek), _DayName);
          }
          catch (ArgumentException) {
            _Day = DayOfWeek.Sunday;
          }
        }
        _Day = (DayOfWeek)EditorGUILayout.EnumPopup("Day", _Day);
        _DayName = _Day.ToString();
      }
      #endif
    }
  }
}

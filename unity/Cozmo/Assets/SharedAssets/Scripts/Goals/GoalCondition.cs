using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;
using System.Linq;
using System.Reflection;
using System.ComponentModel;

#if UNITY_EDITOR
using UnityEditor;
#endif

/// <summary>
/// Goal condition, checked during generation to determine whether or not we can select a daily goal from the pool.
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public abstract class GoalCondition {

      public enum ComparisonType {
        GREATER,
        LESS,
        EQUAL,
        GREATER_INCLUSIVE,
        LESS_INCLUSIVE,
        NOT
      }


      /// <summary>
      /// Compares numerical Condition Values
      /// </summary>
      /// <returns><c>true</c> Compare type for Current value compared to target is met <c>false</c> otherwise.</returns>
      /// <param name="a">Current Value to check.</param>
      /// <param name="b">Target Value to check against.</param>
      /// <param name="compareType">Compare type.</param>
      public bool CompareConditionValues(float a, float b, ComparisonType compareType) {
        switch (compareType) {
        case ComparisonType.GREATER:
          return a > b;
        case ComparisonType.LESS:
          return a < b;
        case ComparisonType.EQUAL:
          return a == b;
        case ComparisonType.GREATER_INCLUSIVE:
          return a >= b;
        case ComparisonType.LESS_INCLUSIVE:
          return a <= b;
        case ComparisonType.NOT:
          return a != b;
        default:
          return false;
        }
      }

      // Override with unique checks to see if certain conditions are met by the current game state
      public virtual bool ConditionMet(GameEventWrapper cozEvent = null) {
        return true;
      }
#if UNITY_EDITOR
      private GUIContent _Label;

      public GUIContent Label {
        get {
          if (_Label == null) {
            var description = GetType().GetCustomAttributes(typeof(DescriptionAttribute), true)
              .Cast<DescriptionAttribute>()
              .Select(x => x.Description)
              .FirstOrDefault() ?? string.Empty;
            var name = GetType().Name.ToHumanFriendly();
            _Label = new GUIContent(name, description);
          }
          return _Label;
        }
      }
#endif

#if UNITY_EDITOR
      // Function to draw the controls for this Condition
      public virtual void OnGUI_DrawUniqueControls() {
        EditorGUILayout.BeginVertical();
        GUI.Label(EditorGUILayout.GetControlRect(), Label);
        DrawControls();
        EditorGUILayout.EndVertical();
      }

      public abstract void DrawControls();

#endif
    }
  }
}

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

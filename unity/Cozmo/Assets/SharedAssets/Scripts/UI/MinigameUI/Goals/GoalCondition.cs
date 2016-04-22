using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;
using System.Linq;
using System.Reflection;
using System.ComponentModel;
using UnityEditor;

/// <summary>
/// Goal condition, checked during generation to determine whether or not we can select a daily goal from the pool.
/// TODO: Write generic JSON converter for Goal Conditions. Currently we cannot properly serialize information about
/// classes that inherit from this.
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


      // Function to draw the controls for this Condition/Action
      public virtual void OnGUI() {

        var rect = EditorGUILayout.GetControlRect();
        EditorGUILayout.BeginVertical();

        GUI.Label(rect, Label);

        EditorGUI.indentLevel++;
        DrawControls();
        EditorGUI.indentLevel--;

        EditorGUILayout.EndVertical();
      }

      public abstract void DrawControls();
    }
  }
}

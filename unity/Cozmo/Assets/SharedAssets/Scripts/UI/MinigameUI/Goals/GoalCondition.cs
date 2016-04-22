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
/// TODO: Write generic JSON converter for Goal Conditions.
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

      private System.Reflection.FieldInfo[] _Fields;

      private System.Reflection.FieldInfo[] Fields { 
        get {
          if (_Fields == null) {
            _Fields = GetType().GetFields(System.Reflection.BindingFlags.Public |
            System.Reflection.BindingFlags.Instance)
              .Where(x => x.FieldType == typeof(int) ||
            x.FieldType == typeof(float) ||
            x.FieldType == typeof(bool) ||
            x.FieldType == typeof(string) ||
            typeof(Enum).IsAssignableFrom(x.FieldType))
              .ToArray();
          }
          return _Fields;
        }
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

      // Some conditions/actions don't have any parameters, so we don't need to draw the foldout
      protected bool _Expandable {
        get {
          return Fields.Length > 0;
        }
      }

      private bool _Expanded = false;


      // Function to draw the controls for this Condition/Action
      public virtual void OnGUI() {

        var rect = EditorGUILayout.GetControlRect();
        EditorGUILayout.BeginVertical();
        // if this condition/action is expandable, draw a foldout. Otherwise just draw a label
        if (_Expandable) {
          _Expanded = EditorGUI.Foldout(rect, _Expanded, Label);
        }
        else {
          rect.x += (EditorGUI.indentLevel + 1) * 15;
          rect.width -= (EditorGUI.indentLevel + 1) * 15;
          GUI.Label(rect, Label);
        }


        // if this condition/action is expanded, Draw the Controls for it
        if (_Expanded) {
          EditorGUI.indentLevel++;
          DrawControls();
          EditorGUI.indentLevel--;
        }

        EditorGUILayout.EndVertical();
      }

      public virtual void DrawControls() {
        
      }
    }
  }
}

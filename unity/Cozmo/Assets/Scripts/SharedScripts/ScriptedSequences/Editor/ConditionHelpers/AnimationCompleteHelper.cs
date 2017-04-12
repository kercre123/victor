using UnityEngine;
using System.Collections;
using ScriptedSequences.Conditions;
using System.Collections.Generic;
using System;
using UnityEditor;
using System.Linq;

namespace ScriptedSequences.Editor.ConditionsHelpers {

  [ScriptedSequenceHelper(typeof(AnimationComplete))]
  public class AnimationCompleteHelper : ScriptedSequenceHelper<AnimationComplete, ScriptedSequenceCondition> {

    public AnimationCompleteHelper(AnimationComplete condition, ScriptedSequenceEditor editor, List<ScriptedSequenceCondition> list) : base(condition, editor, list) { }
    public AnimationCompleteHelper(AnimationComplete condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceCondition> replaceConditions) : base(condition, editor, replaceConditions) { }

    private static string[] _Options;

    static AnimationCompleteHelper() {
      _Options = typeof(Anki.Cozmo.AnimationTrigger).GetFields(System.Reflection.BindingFlags.Static | System.Reflection.BindingFlags.Public).Where(f => f.FieldType == typeof(string)).Select(f => (string)f.GetValue(null)).ToArray();
    }

    protected override void DrawControls(Vector2 mousePosition, EventType eventType) {

      Value.AnyAnimation = EditorGUILayout.Toggle(
        new GUIContent("Any Animation",
          "If this condition is complete as soon as any animation completes. If not must be a specific animation"),
        Value.AnyAnimation);

      if (!Value.AnyAnimation) {
        Value.AnimationName = _Options[EditorGUILayout.Popup("Animation Name", Mathf.Max(0, Array.IndexOf(_Options, Value.AnimationName)), _Options)];
      }
    }
  }
}

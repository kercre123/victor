using UnityEngine;
using System.Collections;
using ScriptedSequences.Actions;
using System.Collections.Generic;
using System;
using UnityEditor;
using System.Linq;

namespace ScriptedSequences.Editor.ActionHelpers {

  [ScriptedSequenceHelper(typeof(PlayCozmoAnimation))]
  public class PlayCozmoAnimationHelper : ScriptedSequenceHelper<PlayCozmoAnimation, ScriptedSequenceAction> {

    public PlayCozmoAnimationHelper(PlayCozmoAnimation condition, ScriptedSequenceEditor editor, List<ScriptedSequenceAction> list) : base(condition, editor, list) {}
    public PlayCozmoAnimationHelper(PlayCozmoAnimation condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceAction> replaceAction) : base(condition, editor, replaceAction) {}

    private static string[] _Options;

    static PlayCozmoAnimationHelper()
    {
      _Options = typeof(AnimationName).GetFields(System.Reflection.BindingFlags.Static | System.Reflection.BindingFlags.Public).Where(f => f.FieldType == typeof(string)).Select(f => (string)f.GetValue(null)).ToArray();
    }

    protected override void DrawControls(Vector2 mousePosition, EventType eventType) {
      Value.AnimationName = _Options[EditorGUILayout.Popup("Animation Name", Mathf.Max(0, Array.IndexOf(_Options, Value.AnimationName)), _Options)];
      Value.WaitToEnd = EditorGUILayout.Toggle(
        new GUIContent("Wait To End", 
                       "If this action should be marked complete when the animation has finished, or immediately."), 
        Value.WaitToEnd);
    }
  }
}

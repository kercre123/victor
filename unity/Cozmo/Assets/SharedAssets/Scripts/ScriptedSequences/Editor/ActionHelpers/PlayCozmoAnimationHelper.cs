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

    public PlayCozmoAnimationHelper(PlayCozmoAnimation condition, ScriptedSequenceEditor editor, List<ScriptedSequenceAction> list) : base(condition, editor, list) { }
    public PlayCozmoAnimationHelper(PlayCozmoAnimation condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceAction> replaceAction) : base(condition, editor, replaceAction) { }

    protected override void DrawControls(Vector2 mousePosition, EventType eventType) {
      //var options = AnimationGroupEditor.AnimationNameOptions;
      Value.AnimationName = Anki.Cozmo.AnimationTrigger.Count;
      //Value.AnimationName = options[EditorGUILayout.Popup("Animation Name", Mathf.Max(0, Array.IndexOf(options, Value.AnimationName)), options)];

      Value.LoopForever = EditorGUILayout.Toggle(
        new GUIContent("Loop Forever",
          "If this action should continuously play the same animation until the node exit condition is met"),
        Value.LoopForever);

      if (!Value.LoopForever) {
        Value.WaitToEnd = EditorGUILayout.Toggle(
          new GUIContent("Wait To End",
            "If this action should be marked complete when the animation has finished, or immediately."),
          Value.WaitToEnd);
      }
    }
  }
}

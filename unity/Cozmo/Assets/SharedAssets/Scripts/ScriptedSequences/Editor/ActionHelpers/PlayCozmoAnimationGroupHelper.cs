using UnityEngine;
using System.Collections;
using ScriptedSequences.Actions;
using System.Collections.Generic;
using System;
using UnityEditor;
using System.Linq;

namespace ScriptedSequences.Editor.ActionHelpers {

  [ScriptedSequenceHelper(typeof(PlayCozmoAnimationGroup))]
  public class PlayCozmoAnimationGroupHelper : ScriptedSequenceHelper<PlayCozmoAnimationGroup, ScriptedSequenceAction> {

    public PlayCozmoAnimationGroupHelper(PlayCozmoAnimationGroup condition, ScriptedSequenceEditor editor, List<ScriptedSequenceAction> list) : base(condition, editor, list) { }
    public PlayCozmoAnimationGroupHelper(PlayCozmoAnimationGroup condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceAction> replaceAction) : base(condition, editor, replaceAction) { }

    protected override void DrawControls(Vector2 mousePosition, EventType eventType) {
      //var options = AnimationGroupEditor.AnimationGroupNameOptions;
      Value.AnimationGroupName = Anki.Cozmo.AnimationTrigger.Count;
      //Value.AnimationGroupName = options[EditorGUILayout.Popup("AnimationGroup Name", Mathf.Max(0, Array.IndexOf(options, Value.AnimationGroupName)), options)];

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

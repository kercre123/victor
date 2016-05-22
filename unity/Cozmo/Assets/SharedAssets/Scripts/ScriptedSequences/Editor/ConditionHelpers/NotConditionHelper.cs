using UnityEngine;
using System.Collections;
using ScriptedSequences.Conditions;
using System;
using System.Collections.Generic;
using System.Linq;
using UnityEditor;

namespace ScriptedSequences.Editor.ConditionHelpers {

  [ScriptedSequenceHelper(typeof(NotCondition))]
  public class NotConditionHelper : ScriptedSequenceHelper<NotCondition, ScriptedSequenceCondition> {

    protected override bool _Expandable {
      get {
        return true;
      }
    }

    public NotConditionHelper(NotCondition condition, ScriptedSequenceEditor editor, List<ScriptedSequenceCondition> list) : base(condition, editor, list){}
    public NotConditionHelper(NotCondition condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceCondition> replaceAction) : base(condition, editor, replaceAction) {}

    protected override void DrawControls(Vector2 mousePosition, EventType eventType) {
      EditorGUI.indentLevel++;
      _Editor.DrawConditionOrActionEntry("Not Condition", Value.Condition, x => { Value.Condition = x; }, mousePosition, eventType);
      EditorGUI.indentLevel--;
    }
  }
}

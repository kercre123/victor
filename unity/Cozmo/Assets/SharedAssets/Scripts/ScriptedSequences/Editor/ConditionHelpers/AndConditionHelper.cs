using UnityEngine;
using System.Collections;
using ScriptedSequences.Conditions;
using System;
using System.Collections.Generic;
using System.Linq;
using UnityEditor;

namespace ScriptedSequences.Editor.ConditionHelpers {

  [ScriptedSequenceHelper(typeof(AndCondition))]
  public class AndConditionHelper : ScriptedSequenceHelper<AndCondition, ScriptedSequenceCondition> {

    protected override bool _Expandable {
      get {
        return true;
      }
    }

    public AndConditionHelper(AndCondition condition, ScriptedSequenceEditor editor, List<ScriptedSequenceCondition> list) : base(condition, editor, list) {}
    public AndConditionHelper(AndCondition condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceCondition> replaceAction) : base(condition, editor, replaceAction) {}

    protected override void DrawControls(Vector2 mousePosition, EventType eventType) {
      _Editor.DrawConditionOrActionList("And Conditions", Value.Conditions, mousePosition, eventType);
    }
  }
}

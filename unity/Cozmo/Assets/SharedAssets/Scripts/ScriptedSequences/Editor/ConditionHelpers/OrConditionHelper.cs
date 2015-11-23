using UnityEngine;
using System.Collections;
using ScriptedSequences.Conditions;
using System;
using System.Collections.Generic;
using System.Linq;
using UnityEditor;

namespace ScriptedSequences.Editor.ConditionHelpers {

  [ScriptedSequenceHelper(typeof(OrCondition))]
  public class OrConditionHelper : ScriptedSequenceHelper<OrCondition, ScriptedSequenceCondition> {

    protected override bool _Expandable {
      get {
        return true;
      }
    }

    public OrConditionHelper(OrCondition condition, ScriptedSequenceEditor editor, List<ScriptedSequenceCondition> list) : base(condition, editor, list) {}
    public OrConditionHelper(OrCondition condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceCondition> replaceAction) : base(condition, editor, replaceAction) {}

    protected override void DrawControls(Vector2 mousePosition, EventType eventType) {
      _Editor.DrawConditionOrActionList("Or Conditions", Value.Conditions, mousePosition, eventType);
    }
  }
}

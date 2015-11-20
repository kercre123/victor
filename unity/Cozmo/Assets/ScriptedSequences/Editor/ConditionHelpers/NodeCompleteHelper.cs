using UnityEngine;
using System.Collections;
using ScriptedSequences.Conditions;
using System;
using System.Collections.Generic;
using System.Linq;
using UnityEditor;

namespace ScriptedSequences.Editor.ConditionHelpers {

  [ScriptedSequenceHelper(typeof(NodeComplete))]
  public class NodeCompleteHelper : ScriptedSequenceHelper<NodeComplete, ScriptedSequenceCondition> {

    protected override bool _Expandable {
      get {
        return true;
      }
    }

    public NodeCompleteHelper(NodeComplete condition, ScriptedSequenceEditor editor, List<ScriptedSequenceCondition> list) : base(condition, editor, list) {}
    public NodeCompleteHelper(NodeComplete condition, ScriptedSequenceEditor editor, Action<ScriptedSequenceCondition> replaceAction) : base(condition, editor, replaceAction){}

    protected override void DrawControls(Vector2 mousePosition, EventType eventType) {
      var sequence = _Editor.target as ScriptedSequence;

      var labels = sequence.Nodes.Select(x => x.Name).ToArray();
      var ids = sequence.Nodes.Select(x => (int)x.Id).ToArray();

      Value.NodeId = (uint)EditorGUILayout.IntPopup("Node", (int)Value.NodeId, labels, ids);

      Value.State = (NodeComplete.CompletionState)EditorGUILayout.EnumPopup("Completion State", Value.State);
    }
  }
}

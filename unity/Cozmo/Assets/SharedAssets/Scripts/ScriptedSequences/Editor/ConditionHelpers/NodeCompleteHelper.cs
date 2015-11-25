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
      var sequence = _Editor.CurrentSequence as ScriptedSequence;

      var labels = sequence.Nodes.Select(x => new GUIContent(x.Name)).ToArray();
      var ids = sequence.Nodes.Select(x => (int)x.Id).ToArray();

      Value.NodeId = (uint)EditorGUILayout.IntPopup(
        new GUIContent("Node", "Which node this condition waits on"), 
        (int)Value.NodeId, labels, ids);

      Value.State = (NodeComplete.CompletionState)EditorGUILayout.EnumPopup(
        new GUIContent("Completion State",
                        "Required State for the node that just completed to be in for this condition to be met."), 
        Value.State);
    }
  }
}

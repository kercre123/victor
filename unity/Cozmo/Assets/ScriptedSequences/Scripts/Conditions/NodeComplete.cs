using System;

namespace ScriptedSequences.Conditions {
  public class NodeComplete : ScriptedSequenceCondition {

    public uint NodeId;

    #region implemented abstract members of ScriptedSequenceCondition
    protected override void EnableChanged(bool enabled) {
      var node = _Parent.GetNode(NodeId);

      if (node == null) {
        DAS.Error(this, "Invalid Node Id specified in NodeComplete Condition! The condition will never be complete!");
        return;
      }

      if (enabled) {
        if (node.IsComplete) {
          IsMet = true;
        } else {
          node.OnComplete += HandleNodeComplete;
        }
      } else {
        node.OnComplete -= HandleNodeComplete;
      }
    }
    #endregion

    private void HandleNodeComplete()
    {
      IsMet = true;
    }

  }
}


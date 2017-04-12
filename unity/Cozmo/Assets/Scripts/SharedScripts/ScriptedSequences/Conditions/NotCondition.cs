using System;
using System.Collections.Generic;

namespace ScriptedSequences.Conditions {
  public class NotCondition : ScriptedSequenceCondition, IScriptedSequenceParent {

    public ScriptedSequenceCondition Condition;

    public override void Initialize(IScriptedSequenceParent parent) {
      base.Initialize(parent);

      if (Condition == null) {
        DAS.Error(this, "NotCondition has no inner condition!");
      }
      Condition.Initialize(this);
      Condition.OnConditionChanged += HandleConditionsChanged;
    }

    protected override void EnableChanged(bool enabled) {
      Condition.IsEnabled = enabled;

      if (enabled) {
        IsMet = !Condition.IsMet;
      }
    }

    private void HandleConditionsChanged() {
      if (IsEnabled) {
        IsMet = !Condition.IsMet;
      }
    }

    public ScriptedSequence GetSequence() {
      return _Parent.GetSequence();
    }

    public ScriptedSequenceNode GetNode(uint id) {
      return _Parent.GetNode(id);
    }
  }
}


using System;
using System.Collections.Generic;

namespace ScriptedSequences.Conditions {
  public class OrCondition : ScriptedSequenceCondition, IScriptedSequenceParent {

    public List<ScriptedSequenceCondition> Conditions = new List<ScriptedSequenceCondition>();

    public override void Initialize(IScriptedSequenceParent parent) {
      base.Initialize(parent);

      for (int i = 0; i < Conditions.Count; i++) {
        Conditions[i].Initialize(this);
        Conditions[i].OnConditionChanged += HandleConditionsChanged;
      }
    }

    protected override void EnableChanged(bool enabled) {
      IsMet = UpdateConditions(enabled);
    }

    private void HandleConditionsChanged() {
      if (IsEnabled) {
        IsMet = UpdateConditions(true);
      }
    }

    private bool UpdateConditions(bool enabled)
    {
      bool anyMet = false;
      for (int i = 0; i < Conditions.Count; i++) {
        var condition = Conditions[i];
        condition.IsEnabled = enabled;

        anyMet = condition.IsMet;
        if (anyMet) {
          break;
        }
      }
      return anyMet;
    }

    public ScriptedSequence GetSequence() {
      return _Parent.GetSequence();
    }

    public ScriptedSequenceNode GetNode(uint id) {
      return _Parent.GetNode(id);
    }
  }
}


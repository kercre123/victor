using System;
using System.Collections.Generic;
using UnityEngine;

namespace ScriptedSequences {
  public class ScriptedSequence : ScriptableObject, IScriptedSequenceParent {
    public string Name;

    public ScriptedSequenceCondition Condition;

    public List<ScriptedSequenceNode> Nodes = new List<ScriptedSequenceNode>();

    public void Fail(Exception error) {
      DAS.Error(this, error.ToString());
      ResetSequence();
    }

    public void ResetSequence() {
      for (int i = 0; i < Nodes.Count; i++) {
        Nodes[i].Reset();
      }
    }

    public void Initialize()
    {
      bool canEnable = true;
      if (Condition != null) {
        Condition.OnConditionChanged += HandleConditionChanged;
        Condition.IsEnabled = true;
        canEnable = Condition.IsMet;
      }

      ScriptedSequenceNode previous = null;
      for (int i = 0; i < Nodes.Count; i++) {
        Nodes[i].Initialize(this, previous);
        previous = Nodes[i];
      }

      if (canEnable) {
        Enable();
      }
    }

    public ScriptedSequenceNode GetNode(uint id)
    {
      return Nodes.Find(x => x.Id == id);
    }

    public ScriptedSequence GetSequence()
    {
      return this;
    }

    private void HandleConditionChanged()
    {
      if (Condition.IsMet) {
        Enable();
      }
      else {
        ResetSequence();
      }
    }

    private void Enable()
    {
      for (int i = 0; i < Nodes.Count; i++) {
        Nodes[i].TryEnable();
      }
    }
  }
}


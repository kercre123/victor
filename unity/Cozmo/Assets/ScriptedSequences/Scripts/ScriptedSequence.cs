using System;
using System.Collections.Generic;
using UnityEngine;

namespace ScriptedSequences {
  public class ScriptedSequence : IScriptedSequenceParent {
    public string Name;

    public bool RequiresConditionRemainsMet;

    public bool Repeatable;

    public bool CanResume;

    public ScriptedSequenceCondition Condition;

    public List<ScriptedSequenceNode> Nodes = new List<ScriptedSequenceNode>();

    public string DebugName { get { return Name; } }

    public void Fail(Exception error) {
      DAS.Error(this, error.ToString());
      ResetSequence();
    }

    public void ResetSequence() {
      #if DEBUG_SCRIPTED_SEQUENCES
      DAS.Debug(this, "Reset Called on Scripted Sequence " +Name);
      #endif
      for (int i = 0; i < Nodes.Count; i++) {
        Nodes[i].Reset();
      }
    }

    public void Initialize()
    {
      #if DEBUG_SCRIPTED_SEQUENCES
      DAS.Debug(this, "Initialize Called on Scripted Sequence " +Name);
      #endif
      bool canEnable = false;
      if (Condition != null) {
        Condition.Initialize(this);
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
      else if(RequiresConditionRemainsMet) {
        ResetSequence();
      }
    }

    public void Enable()
    {
      #if DEBUG_SCRIPTED_SEQUENCES
      DAS.Debug(this, "Enable Called on Scripted Sequence " +Name);
      #endif
      for (int i = 0; i < Nodes.Count; i++) {
        Nodes[i].TryEnable();
      }
    }
  }
}


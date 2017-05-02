using System;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft.Json;
using System.Linq;
using System.ComponentModel;

namespace ScriptedSequences {
  public class ScriptedSequence : IScriptedSequenceParent {
    public string Name;

    public bool RequiresConditionRemainsMet;

    [DefaultValue(true)]
    public bool ActivateOnConditionMet = true;

    public bool Repeatable;

    public ScriptedSequenceCondition Condition;

    public List<ScriptedSequenceNode> Nodes = new List<ScriptedSequenceNode>();

    public event Action OnComplete;

    public event Action<Exception> OnError;

    private bool _IsComplete;

    [JsonIgnore]
    public bool IsComplete {
      get { return _IsComplete; }
      private set {
        if (value && !Repeatable) {
          DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CompletedScriptedSequences[Name] = true;
          DataPersistence.DataPersistenceManager.Instance.Save();
        }
        _IsComplete = value;
      }
    }

    [JsonIgnore]
    public string DebugName { get { return Name; } }

    public void Fail(Exception error) {
      DAS.Error(this, error.ToString());

      if (OnError != null) {
        OnError(error);
      }

      ResetSequence();
    }

    public void ResetSequence() {
#if DEBUG_SCRIPTED_SEQUENCES
      DAS.Debug(this, "Reset Called on Scripted Sequence " +Name);
#endif

      for (int i = 0; i < Nodes.Count; i++) {
        Nodes[i].Reset();
      }

      if (Condition != null) {
        // Toggling Condition.IsEnabled should reset our sequence to its starting state
        Condition.IsEnabled = false;

        if (Repeatable || !IsComplete) {
          IsComplete = false;
          Condition.IsEnabled = true;
          if (Condition.IsMet && ActivateOnConditionMet) {
            Enable();
          }
        }
      }
    }

    public void Complete() {
      IsComplete = true;

      if (OnComplete != null) {
        OnComplete();
      }

      ResetSequence();
    }

    public void Initialize() {
      if (!Repeatable) {
        DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CompletedScriptedSequences.TryGetValue(Name, out _IsComplete);
      }

#if DEBUG_SCRIPTED_SEQUENCES
      DAS.Debug(this, "Initialize Called on Scripted Sequence " +Name);
#endif
      bool canEnable = false;
      if (Condition != null) {
        Condition.Initialize(this);
        Condition.OnConditionChanged += HandleConditionChanged;
        Condition.IsEnabled = !IsComplete;
        canEnable = Condition.IsMet && ActivateOnConditionMet;
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

    public ScriptedSequenceNode GetNode(uint id) {
      return Nodes.Find(x => x.Id == id);
    }

    public ScriptedSequence GetSequence() {
      return this;
    }

    private void HandleConditionChanged() {
      if (Condition.IsMet && ActivateOnConditionMet) {
        Enable();
      }
      else if (RequiresConditionRemainsMet && !IsComplete) {
        Fail(new Exception("Condition Became Unmet"));
      }
    }

    public void Enable() {
#if DEBUG_SCRIPTED_SEQUENCES
      DAS.Debug(this, "Enable Called on Scripted Sequence " +Name);
#endif
      for (int i = 0; i < Nodes.Count; i++) {
        Nodes[i].TryEnable();
      }
    }

    public List<ScriptedSequenceNode> GetActiveNodes() {
      return Nodes.Where(x => x.IsActive).ToList();
    }

    public List<ScriptedSequenceNode> GetWaitingNodes() {
      return Nodes.Where(x => x.IsEnabled && !x.IsComplete && !x.IsActive).ToList();
    }
  }
}


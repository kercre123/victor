using System;
using System.Collections.Generic;
using UnityEngine;
using System.Linq;

namespace ScriptedSequences
{
  [System.Serializable]
  public class ScriptedSequenceNode : IScriptedSequenceParent 
	{
    public string Name;

    public uint Id;

    public bool Sequencial = true;

    public bool Final;

    public bool CheckPoint;

    public bool FailOnError;

    public List<ScriptedSequenceCondition> Conditions = new List<ScriptedSequenceCondition>();

    public List<ScriptedSequenceAction> Actions = new List<ScriptedSequenceAction>();

    [NonSerialized]
    private ScriptedSequence _Parent;

    [NonSerialized]
    private ScriptedSequenceNode _Previous;

    public string DebugName { get { return _Parent.DebugName + "::" + Name; } }

    [NonSerialized]
    private bool _IsComplete;

    public bool IsComplete { 
      get { return _IsComplete; } 
      private set { 
        if (value && !_IsComplete) {
          #if DEBUG_SCRIPTED_SEQUENCES
          DAS.Debug(this, DebugName +" Is Now Complete!");
          #endif
          if (OnComplete != null) {
            OnComplete();
          }
        }
        _IsComplete = value;
      }
    }

    [NonSerialized]
    private IAsyncToken _ActToken;
    public bool IsActive { get { return _ActToken != null && !_ActToken.IsReady; } }

    [NonSerialized]
    private bool _IsEnabled;

    public bool IsEnabled { 
      get {
        return _IsEnabled;
      }

      private set {
        _IsEnabled = value;

        #if DEBUG_SCRIPTED_SEQUENCES
        DAS.Debug(this, DebugName +" Is "+(_IsEnabled ? "Enabled" : "Disabled")+"!");
        #endif
        if (UpdateConditions(_IsEnabled)) {
          Act();
        }
      }
    }

    public event Action OnComplete;

    public void Initialize(ScriptedSequence parent, ScriptedSequenceNode previous) {
      _Parent = parent;

      for (int i = 0; i < Conditions.Count; i++) {
        Conditions[i].Initialize(this);
        Conditions[i].OnConditionChanged += HandleConditionsChanged;
      }

      for (int i = 0; i < Actions.Count; i++) {
        Actions[i].Initialize(this);
      }

      if (Sequencial && previous != null) {
        _Previous = previous;
        previous.OnComplete += HandlePreviousNodeComplete;
      }
    }

    public void TryEnable()
    {      
      #if DEBUG_SCRIPTED_SEQUENCES
      DAS.Debug(this, "TryEnable Called on " + DebugName);
      #endif
      if (_Previous != null && !_Previous.IsComplete) {
        return;
      }

      IsEnabled = true;
    }

    public void Reset()
    {
      #if DEBUG_SCRIPTED_SEQUENCES
      DAS.Debug(this, "Reset Called on " + DebugName);
      #endif
      if (_ActToken != null) {
        _ActToken.Abort();
        _ActToken = null;
      }

      IsComplete = false;
      IsEnabled = false;
    }

    public ScriptedSequenceNode GetNode(uint id)
    {
      return _Parent.GetNode(id);
    }

    public ScriptedSequence GetSequence()
    {
      return _Parent;
    }

    private void HandlePreviousNodeComplete()
    {
      IsEnabled = true;
    }

    private void HandleConditionsChanged()
    {
      if (UpdateConditions(IsEnabled)) {
        Act();
      }
    }

    private bool UpdateConditions(bool enabled)
    {
      if (IsComplete || IsActive) {
        return false;
      }

      bool lastMet = enabled;
      for (int i = 0; i < Conditions.Count; i++) {
        var condition = Conditions[i];
        condition.IsEnabled = lastMet;

        lastMet = condition.IsMet;
      }
      return lastMet;
    }

    public void Act() {
      #if DEBUG_SCRIPTED_SEQUENCES
      DAS.Debug(this, "Act Called on "+DebugName);
      #endif
      var actingTokens = new ISimpleAsyncToken[Actions.Count];

      for (int i = 0; i < Actions.Count; i++) {
        #if DEBUG_SCRIPTED_SEQUENCES
        DAS.Debug(this, "Calling Act on " + Actions[i].DebugName);
        #endif
        actingTokens[i] = Actions[i].Act();
      }
      if (FailOnError) {
        var token = SimpleAsyncToken.Reduce(actingTokens);
        _ActToken = token;
        token.Ready(result => {

          if (!result.Success) {
            _Parent.Fail(result.Error);
            return;
          }
          IsComplete = true;
        });
      } else {
        var token = SimpleAsyncToken.PessimisticReduce(actingTokens);
        _ActToken = token;
        token.Ready(result => {
          IsComplete = true;
        });
      }
    }
  }

}


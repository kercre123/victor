using System;
using UnityEngine;
using Newtonsoft.Json;

namespace ScriptedSequences
{
  public abstract class ScriptedSequenceCondition : IScriptedSequenceItem
	{
    [JsonIgnore]
    protected IScriptedSequenceParent _Parent { get; private set; }

    [JsonIgnore]
    public string DebugName { get { return _Parent.DebugName + "::" + GetType().Name; } }

    public virtual void Initialize(IScriptedSequenceParent parent)
    {
      _IsEnabled = false;
      _IsMet = false;
      _Parent = parent;
    }

    private bool _IsEnabled;
    [JsonIgnore]
    public bool IsEnabled { 
      get { return _IsEnabled; } 
      set { 
        if (value != _IsEnabled) {
          #if DEBUG_SCRIPTED_SEQUENCES
          DAS.Debug(this, "Condition " + DebugName + " Is "+(value ? "Enabled" : "Disabled")+"!");
          #endif
          _IsEnabled = value;
          EnableChanged(_IsEnabled);
          if (!_IsEnabled) {
            IsMet = false;
          }
        }
      }
    }

    public event Action OnConditionChanged;

    private bool _IsMet;
    [JsonIgnore]
    public bool IsMet { 
      get { return _IsMet; } 
      protected set { 
        if (_IsMet != value) {
          #if DEBUG_SCRIPTED_SEQUENCES
          DAS.Debug(this, "Condition " + DebugName +" Is "+(value ? "Met" : "Not Met")+"!");
          #endif
          _IsMet = value;
          if (OnConditionChanged != null) {
            OnConditionChanged();
          }
        }
      }
    }

    protected abstract void EnableChanged(bool enabled);
	}
}


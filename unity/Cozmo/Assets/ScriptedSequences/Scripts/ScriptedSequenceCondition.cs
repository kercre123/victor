using System;
using UnityEngine;

namespace ScriptedSequences
{
	public abstract class ScriptedSequenceCondition : ScriptableObject
	{
    protected IScriptedSequenceParent _Parent { get; private set; }

    public string DebugName { get { return _Parent.DebugName + "::" + GetType().Name; } }

    public virtual void Initialize(IScriptedSequenceParent parent)
    {
      _IsEnabled = false;
      _IsMet = false;
      _Parent = parent;
    }

    [NonSerialized]
    private bool _IsEnabled;
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

    [NonSerialized]
    private bool _IsMet;
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


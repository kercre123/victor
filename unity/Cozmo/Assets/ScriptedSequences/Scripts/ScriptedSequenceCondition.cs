using System;
using UnityEngine;

namespace ScriptedSequences
{
	public abstract class ScriptedSequenceCondition : ScriptableObject
	{
    protected IScriptedSequenceParent _Parent { get; private set; }

    public virtual void Initialize(IScriptedSequenceParent parent)
    {
      _Parent = parent;
    }

    private bool _IsEnabled;
    public bool IsEnabled { 
      get { return _IsEnabled; } 
      set { 
        if (value != _IsEnabled) {
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
    public bool IsMet { 
      get { return _IsMet; } 
      protected set { 
        if (_IsMet != value) {
          if (OnConditionChanged != null) {
            OnConditionChanged();
          }
        }
        _IsMet = value;
      }
    }

    protected abstract void EnableChanged(bool enabled);
	}
}


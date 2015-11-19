using System;
using UnityEngine;

namespace ScriptedSequences {
  public abstract class ScriptedSequenceAction : ScriptableObject {
    protected IScriptedSequenceParent _Parent { get; private set; }

    public void Initialize(IScriptedSequenceParent parent)
    {
      _Parent = parent;
    }

    public abstract ISimpleAsyncToken Act();
  }
}


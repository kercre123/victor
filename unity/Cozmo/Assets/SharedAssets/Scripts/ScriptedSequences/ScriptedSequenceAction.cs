using System;
using UnityEngine;
using Newtonsoft.Json;

namespace ScriptedSequences {
  public abstract class ScriptedSequenceAction : IScriptedSequenceItem {
    [JsonIgnore]
    protected IScriptedSequenceParent _Parent { get; private set; }

    [JsonIgnore]
    public string DebugName { get { return _Parent.DebugName + "::" + GetType().Name; } }

    public void Initialize(IScriptedSequenceParent parent)
    {
      _Parent = parent;
    }

    public abstract ISimpleAsyncToken Act();
  }
}


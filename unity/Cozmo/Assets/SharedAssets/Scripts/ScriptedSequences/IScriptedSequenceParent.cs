using System;

namespace ScriptedSequences {
  public interface IScriptedSequenceParent {

    ScriptedSequence GetSequence();

    ScriptedSequenceNode GetNode(uint id);

    string DebugName { get; }
  }
}


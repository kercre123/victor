using System;

namespace ScriptedSequences.Actions {
  public class DebugLogAction : ScriptedSequenceAction {

    public string StringToLog;

    public override ISimpleAsyncToken Act() {
      DAS.Debug(this, StringToLog);
      return new SimpleAsyncToken(true);
    }
  }
}


using System;

namespace ScriptedSequences.Actions {
  public class UnblockInput : ScriptedSequenceAction {
    #region implemented abstract members of ScriptedSequenceAction
    public override ISimpleAsyncToken Act() {
      UIManager.Instance.HideTouchCatcher();
      return new SimpleAsyncToken(true);
    }
    #endregion

  }
}


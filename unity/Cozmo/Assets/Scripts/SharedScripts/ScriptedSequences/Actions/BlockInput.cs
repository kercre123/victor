using System;

namespace ScriptedSequences.Actions {
  public class BlockInput : ScriptedSequenceAction {
    #region implemented abstract members of ScriptedSequenceAction
    public override ISimpleAsyncToken Act() {
      UIManager.Instance.ShowTouchCatcher();
      return new SimpleAsyncToken(true);
    }
    #endregion
    
  }
}


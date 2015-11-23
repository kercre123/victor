using UnityEngine;
using System.Collections;

namespace ScriptedSequences.Actions {
  public class StartConversation : ScriptedSequenceAction {
    #region implemented abstract members of ScriptedSequenceAction
    public override ISimpleAsyncToken Act() {
      Conversations.ConversationManager.Instance.StartNewConversation();
      return new SimpleAsyncToken(true);
    }
    #endregion
    
  }
}

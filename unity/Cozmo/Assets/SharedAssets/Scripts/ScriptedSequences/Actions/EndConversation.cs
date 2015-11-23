using UnityEngine;
using System.Collections;

namespace ScriptedSequences.Actions {
  public class EndConversation : ScriptedSequenceAction {
    #region implemented abstract members of ScriptedSequenceAction
    public override ISimpleAsyncToken Act() {
      Conversations.ConversationManager.Instance.SaveConversationToHistory();
      return new SimpleAsyncToken(true);
    }
    #endregion

  }
}

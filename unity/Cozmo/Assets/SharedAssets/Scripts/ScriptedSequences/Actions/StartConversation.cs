using UnityEngine;
using System.Collections;
using System.ComponentModel;

namespace ScriptedSequences.Actions {
  public class StartConversation : ScriptedSequenceAction {
    #region implemented abstract members of ScriptedSequenceAction

    [Description("Conversation Name is used to categorize dialog to make it easier to search")]
    public string ConversationName;

    public override ISimpleAsyncToken Act() {
      Conversations.ConversationManager.Instance.StartNewConversation(ConversationName);
      return new SimpleAsyncToken(true);
    }

    #endregion
    
  }
}

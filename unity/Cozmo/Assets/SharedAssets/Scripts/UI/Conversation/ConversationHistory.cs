using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {
  public class ConversationHistory {
    private List<Conversation> _Conversations = new List<Conversation>();

    public void AddConversation(Conversation conversation) {
      _Conversations.Add(conversation);
    }
  }
}
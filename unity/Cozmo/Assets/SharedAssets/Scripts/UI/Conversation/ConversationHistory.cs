using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {
  public class ConversationHistory {
    private Dictionary<string, Conversation> _Conversations = new Dictionary<string, Conversation>();

    public void AddConversation(string conversationKey, Conversation conversation) {
      Conversation exisitingConversation;
      if (_Conversations.TryGetValue(conversationKey, out exisitingConversation)) {
        exisitingConversation.AddToConversation(conversation);
      }
      else {
        _Conversations.Add(conversationKey, conversation);
      }

    }
  }
}
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {
  public class ConversationHistory {
    public readonly Dictionary<string, Conversation> Conversations = new Dictionary<string, Conversation>();

    public void AddConversation(string conversationKey, Conversation conversation) {
      Conversation existingConversation;
      if (Conversations.TryGetValue(conversationKey, out existingConversation)) {
        existingConversation.AddToConversation(conversation);
      }
      else {
        Conversations.Add(conversationKey, conversation);
      }

    }
  }
}
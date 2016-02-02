using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {
  public class ConversationHistory {
    public readonly Dictionary<string, Conversation> Conversations = new Dictionary<string, Conversation>();

    public void AddConversation(string conversationKey, Conversation conversation) {
      Conversation exisitingConversation;
      if (Conversations.TryGetValue(conversationKey, out exisitingConversation)) {
        exisitingConversation.AddToConversation(conversation);
      }
      else {
        Conversations.Add(conversationKey, conversation);
      }

    }
  }
}
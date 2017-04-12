using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {
  public class Conversation {
    public readonly List<ConversationLine> ConversationLines = new List<ConversationLine>();

    public void AddToConversation(ConversationLine line) {
      ConversationLines.Add(line);
    }

    public void AddToConversation(Conversation conversation) {
      for (int i = 0; i < conversation.ConversationLines.Count; ++i) {
        ConversationLines.Add(conversation.ConversationLines[i]);
      }
    }

  }
}
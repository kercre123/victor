using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {
  public class Conversation {
    private List<ConversationLine> _ConversationLines = new List<ConversationLine>();

    public void AddToConversation(ConversationLine line) {
      _ConversationLines.Add(line);
    }

    public void AddToConversation(Conversation conversation) {
      for (int i = 0; i < conversation._ConversationLines.Count; ++i) {
        _ConversationLines.Add(conversation._ConversationLines[i]);
      }
    }

  }
}
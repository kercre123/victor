using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Conversations {
  public class Conversation {
    private List<ConversationLine> _ConversationLines = new List<ConversationLine>();

    public void AddToConversation(ConversationLine line) {
      _ConversationLines.Add(line);
    }

  }
}
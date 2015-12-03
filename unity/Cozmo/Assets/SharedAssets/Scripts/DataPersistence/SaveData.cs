using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DataPersistence {
  public class SaveData {
    public Dictionary<string, bool> CompletedScriptedSequences = new Dictionary<string, bool>();

    public Conversations.ConversationHistory ConversationHistory = new Conversations.ConversationHistory();
  }
}
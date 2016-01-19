using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DataPersistence {
  public class SaveData {
    public Dictionary<string, bool> CompletedScriptedSequences = new Dictionary<string, bool>();

    public Conversations.ConversationHistory ConversationHistory = new Conversations.ConversationHistory();

    public List<string> CompletedChallengeIds = new List<string>();

    public List<TimelineEntryData> Sessions = new List<TimelineEntryData>();

    public int FriendshipPoints;

    public int FriendshipLevel;

    public StatContainer CurrentStats;
  }
}
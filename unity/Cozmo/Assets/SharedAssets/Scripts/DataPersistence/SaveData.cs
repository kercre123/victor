using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DataPersistence {
  public class SaveData {
    public Dictionary<string, bool> CompletedScriptedSequences;

    public Conversations.ConversationHistory ConversationHistory;

    public List<string> CompletedChallengeIds;

    public List<TimelineEntryData> Sessions;

    public int FriendshipPoints;

    public int FriendshipLevel;

    public StatContainer CurrentStats;

    public MinigameSaveData MinigameSaveData;

    public SaveData() {
      CompletedScriptedSequences = new Dictionary<string, bool>();
      ConversationHistory = new Conversations.ConversationHistory();
      CompletedChallengeIds = new List<string>();
      Sessions = new List<TimelineEntryData>();
      CurrentStats = new StatContainer();
      MinigameSaveData = new MinigameSaveData();
    }
  }
}
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DataPersistence {
  public class SaveData {
    public Dictionary<string, bool> CompletedScriptedSequences;

    public Conversations.ConversationHistory ConversationHistory;

    public List<TimelineEntryData> Sessions;

    public int FriendshipPoints;

    public int FriendshipLevel;

    public int GreenPoints;

    // TODO: replace with Dictionary<HexType, int>
    public int HexPieces;

    public int TreatCount;

    public Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float> VolumePreferences;

    public SaveData() {
      CompletedScriptedSequences = new Dictionary<string, bool>();
      ConversationHistory = new Conversations.ConversationHistory();
      Sessions = new List<TimelineEntryData>();
      VolumePreferences = new Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float>();
    }
  }
}
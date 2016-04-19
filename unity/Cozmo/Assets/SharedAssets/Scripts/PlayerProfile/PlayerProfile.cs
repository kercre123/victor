using UnityEngine;
using System.Collections;
using System.Collections.Generic;

[System.Serializable]
public class PlayerProfile {
  public Dictionary<string, bool> CompletedScriptedSequences;

  public Conversations.ConversationHistory ConversationHistory;

  public List<DataPersistence.TimelineEntryData> Sessions;

  public int GreenPoints;

  // TODO: replace with Dictionary<HexType, int>
  public int HexPieces;

  public int TreatCount;

  public Dictionary<string, DataPersistence.GameSkillData> CozmoSkillLevels;

  public Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float> VolumePreferences;

  // Bump if introducing breaking changes and compare in DataPersistenceManager Constructor.
  public int SaveVersion;

  public PlayerProfile() {
    CompletedScriptedSequences = new Dictionary<string, bool>();
    ConversationHistory = new Conversations.ConversationHistory();
    Sessions = new List<DataPersistence.TimelineEntryData>();
    VolumePreferences = new Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float>();
    CozmoSkillLevels = new Dictionary<string, DataPersistence.GameSkillData>();
    SaveVersion = 0;
  }
}
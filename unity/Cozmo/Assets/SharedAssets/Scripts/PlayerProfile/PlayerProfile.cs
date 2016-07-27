using UnityEngine;
using System.Collections;
using System.Collections.Generic;

[System.Serializable]
public class PlayerProfile {
  public Dictionary<string, bool> CompletedScriptedSequences;

  public Conversations.ConversationHistory ConversationHistory;

  public List<DataPersistence.TimelineEntryData> Sessions;

  public int CurrentStreak;

  public Cozmo.Inventory Inventory;

  public Dictionary<string, DataPersistence.GameSkillData> CozmoSkillLevels;

  public Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float> VolumePreferences;

  public Dictionary<string, int> GameDifficulty;

  public Dictionary<string, int> TotalGamesPlayed;

  // Bump if introducing breaking changes and compare in DataPersistenceManager Constructor.
  public int SaveVersion;

  public PlayerProfile() {
    CompletedScriptedSequences = new Dictionary<string, bool>();
    GameDifficulty = new Dictionary<string, int>();
    TotalGamesPlayed = new Dictionary<string, int>();
    ConversationHistory = new Conversations.ConversationHistory();
    Sessions = new List<DataPersistence.TimelineEntryData>();
    VolumePreferences = new Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float>();
    CozmoSkillLevels = new Dictionary<string, DataPersistence.GameSkillData>();
    Inventory = new Cozmo.Inventory();
    SaveVersion = 0;
    CurrentStreak = 0;
  }
}

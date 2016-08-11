using UnityEngine;
using System.Collections;
using System.Collections.Generic;

[System.Serializable]
public class PlayerProfile {
  public bool FirstTimeUserFlow;

  public bool ProfileCreated;

  public string ProfileName;

  public System.DateTime Birthdate;

  public Dictionary<string, bool> CompletedScriptedSequences;

  public Conversations.ConversationHistory ConversationHistory;

  public List<DataPersistence.TimelineEntryData> Sessions;

  // this would work better as a HashSet or a list but for some reason
  // our json serializer shits itself if it is a HashSet<string> or a List<string>
  public Dictionary<string, bool> GameInstructionalVideoPlayed;

  public Cozmo.Inventory Inventory;

  public Dictionary<string, DataPersistence.GameSkillData> CozmoSkillLevels;

  public Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float> VolumePreferences;

  public Dictionary<string, int> GameDifficulty;

  public Dictionary<string, int> TotalGamesPlayed;

  public Dictionary<OnboardingManager.OnboardingPhases, int> OnboardingStages;

  // Bump if introducing breaking changes and compare in DataPersistenceManager Constructor.
  public int SaveVersion;

  public PlayerProfile() {
    FirstTimeUserFlow = true;
    ProfileCreated = false;
    CompletedScriptedSequences = new Dictionary<string, bool>();
    GameDifficulty = new Dictionary<string, int>();
    TotalGamesPlayed = new Dictionary<string, int>();
    ConversationHistory = new Conversations.ConversationHistory();
    Sessions = new List<DataPersistence.TimelineEntryData>();
    GameInstructionalVideoPlayed = new Dictionary<string, bool>();
    VolumePreferences = new Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float>();
    CozmoSkillLevels = new Dictionary<string, DataPersistence.GameSkillData>();
    Inventory = new Cozmo.Inventory();
    SaveVersion = 0;
    OnboardingStages = new Dictionary<OnboardingManager.OnboardingPhases, int>();
  }
}

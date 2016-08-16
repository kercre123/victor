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

  public int MaximumStreak;

  /// <summary>
  /// Gets the total daily goals completed across all past sessions.
  /// </summary>
  /// <value>The total daily goals completed.</value>
  public int TotalDailyGoalsCompleted {
    get {
      int total = 0;
      for (int i = 0; i < Sessions.Count; i++) {
        for (int j = 0; j < Sessions[i].DailyGoals.Count; j++) {
          if (Sessions[i].DailyGoals[j].GoalComplete) {
            total++;
          }
        }
      }
      return total;
    }
  }

  /// <summary>
  /// Gets the total number of sessions that you completed every daily goal in across all past sessions.
  /// </summary>
  /// <value>The total full sets of goals completed.</value>
  public int TotalFullSetsOfGoalsCompleted {
    get {
      int total = 0;
      for (int i = 0; i < Sessions.Count; i++) {
        bool allDone = (Sessions[i].DailyGoals.Count > 0);
        for (int j = 0; j < Sessions[i].DailyGoals.Count; j++) {
          if (Sessions[i].DailyGoals[j].GoalComplete == false) {
            allDone = false;
            break;
          }
        }
        if (allDone) {
          total++;
        }
      }
      return total;
    }
  }

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
    MaximumStreak = 0;
    OnboardingStages = new Dictionary<OnboardingManager.OnboardingPhases, int>();
  }
}

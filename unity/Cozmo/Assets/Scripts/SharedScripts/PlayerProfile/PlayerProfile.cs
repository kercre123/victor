using System.Collections.Generic;

namespace DataPersistence {
  [System.Serializable]
  public class PlayerProfile {
    public bool FirstTimeUserFlow;

    public bool DataCollectionEnabled;

    public bool ProfileCreated;

    public string ProfileName;

    public bool FirstTimeFaceEnrollmentHowToPlay;

    public System.DateTime Birthdate;

    public Dictionary<string, bool> CompletedScriptedSequences;

    public List<DataPersistence.TimelineEntryData> Sessions;

    // this would work better as a HashSet or a list but for some reason
    // our json serializer shits itself if it is a HashSet<string> or a List<string>
    public Dictionary<string, bool> GameInstructionalVideoPlayed;

    public Cozmo.Inventory Inventory;

    public Dictionary<string, DataPersistence.GameSkillData> CozmoSkillLevels;

    public Dictionary<string, int> GameDifficulty;
    // Keys for HighScores is ChallengeID+GameDifficulty, not just ID.
    public Dictionary<string, int> HighScores;
    public Dictionary<string, int> LastPlayedDifficulty;
    public Dictionary<string, int> TotalGamesPlayed;

    public int MaximumStreak;
    public int CurrentStreak;
    public int TotalSessions;
    // Just displayed in profile tab, hopefully we can kill this later.
    public int TotalDailyGoalsCompleted;
    // Mainly just for logging, but different from sessions in that you need to connect.
    public int DaysWithCozmo;
    public int CodeLabUserProjectNum;

    public List<DataPersistence.CodeLabProject> CodeLabProjects;

    public List<Anki.Cozmo.UnlockId> NewUnlocks;

    public List<string> PreviousTags;

    public Dictionary<OnboardingManager.OnboardingPhases, int> OnboardingStages;

    public static int kSaveVersionCurrent = 1;
    // Bump if introducing breaking changes and compare in DataPersistenceManager Constructor.
    public int SaveVersion = kSaveVersionCurrent;

    public PlayerProfile() {
      FirstTimeUserFlow = true;
      DataCollectionEnabled = true;
      ProfileCreated = false;
      FirstTimeFaceEnrollmentHowToPlay = true;
      CompletedScriptedSequences = new Dictionary<string, bool>();
      GameDifficulty = new Dictionary<string, int>();
      HighScores = new Dictionary<string, int>();
      TotalGamesPlayed = new Dictionary<string, int>();
      LastPlayedDifficulty = new Dictionary<string, int>();
      Sessions = new List<DataPersistence.TimelineEntryData>();
      GameInstructionalVideoPlayed = new Dictionary<string, bool>();
      CozmoSkillLevels = new Dictionary<string, DataPersistence.GameSkillData>();
      Inventory = new Cozmo.Inventory();
      SaveVersion = kSaveVersionCurrent;
      MaximumStreak = 0;
      CurrentStreak = 0;
      TotalSessions = 0;
      DaysWithCozmo = 0;
      TotalDailyGoalsCompleted = 0;
      CodeLabUserProjectNum = 1;
      OnboardingStages = new Dictionary<OnboardingManager.OnboardingPhases, int>();
      CodeLabProjects = new List<DataPersistence.CodeLabProject>();
      NewUnlocks = new List<Anki.Cozmo.UnlockId>();
      PreviousTags = new List<string>();
    }
  }
}
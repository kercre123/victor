using System.Collections.Generic;

namespace DataPersistence {
  [System.Serializable]
  public class PlayerProfile {
    public bool FirstTimeUserFlow;

    public bool DataCollectionEnabled;

    public bool ProfileCreated;

    public string ProfileName;

    public System.DateTime Birthdate;

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
    // Mainly just for logging, but different from sessions in that you need to connect.
    public int DaysWithCozmo;
    public int CodeLabUserProjectNum;
    public int CodeLabUserProjectNumVertical;
    public bool HideFreeplayCard;
    public bool DroneModeInstructionsSeen;

    public List<DataPersistence.CodeLabProject> CodeLabProjects;
    public int CodeLabHorizontalPlayed;
    public bool CodeLabRemixCreated;

    public List<Anki.Cozmo.UnlockId> NewUnlocks;

    public int DisplayedSparks;
    public int DisplayedStars;

    public List<Anki.Cozmo.ExternalInterface.StarLevelCompleted> NewStarLevels;

    public List<string> PreviousTags;

    public Dictionary<OnboardingManager.OnboardingPhases, int> OnboardingStages;

    public bool OSNotificationsPermissionsPromptShown;
    public System.DateTime LastTimeAskedAboutNotifications;
    public int NumTimesNotificationPermissionReminded;
    public List<Cozmo.Notifications.Notification> NotificationsToBeSent;
    public Cozmo.Notifications.Notification MostRecentNotificationClicked;

    public Anki.Util.AnkiLab.AssignmentDef[] LabAssignments;

    public static int kSaveVersionCurrent = 2;

    // Bump if introducing breaking changes and compare in DataPersistenceManager Constructor.
    public int SaveVersion = kSaveVersionCurrent;

    public int NumTimesToolTipNotifTapped;

    public List<string> WhatsNewFeaturesSeen;

    // $player_id DAS global
    public string PlayerId;

    // $phys DAS global
    public string RobotPhysicalId;
    
    // Track the number of attempts the device has made at the initial loading of resources
    public int NumLoadingAttempts = 0;
        
    public PlayerProfile() {
      FirstTimeUserFlow = true;
      DataCollectionEnabled = true;
      ProfileCreated = false;
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
      CodeLabUserProjectNum = 1;
      CodeLabUserProjectNumVertical = 1;
      HideFreeplayCard = false;
      DroneModeInstructionsSeen = false;
      OnboardingStages = new Dictionary<OnboardingManager.OnboardingPhases, int>();
      CodeLabProjects = new List<DataPersistence.CodeLabProject>();
      CodeLabHorizontalPlayed = 0;
      CodeLabRemixCreated = false;
      NewUnlocks = new List<Anki.Cozmo.UnlockId>();
      DisplayedStars = 0;
      DisplayedSparks = 0;
      NewStarLevels = new List<Anki.Cozmo.ExternalInterface.StarLevelCompleted>();
      PreviousTags = new List<string>();
      LabAssignments = new Anki.Util.AnkiLab.AssignmentDef[0];
#if UNITY_ANDROID && !UNITY_EDITOR
      OSNotificationsPermissionsPromptShown = true;
#else
      OSNotificationsPermissionsPromptShown = false;
#endif
      NumTimesNotificationPermissionReminded = 0;
      LastTimeAskedAboutNotifications = System.DateTime.Now;
      NotificationsToBeSent = new List<Cozmo.Notifications.Notification>();
      MostRecentNotificationClicked = null;
      NumTimesToolTipNotifTapped = 0;
      WhatsNewFeaturesSeen = new List<string>();
#if UNITY_ANDROID && !UNITY_EDITOR
      NumLoadingAttempts = 0;
#endif
    }
  }
}

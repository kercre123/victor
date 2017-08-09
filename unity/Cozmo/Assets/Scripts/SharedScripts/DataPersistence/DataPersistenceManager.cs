using System;
using UnityEngine;
using System.IO;
using Newtonsoft.Json;
using System.Collections.Generic;
using Cozmo.Util;
using Anki.Cozmo;

namespace DataPersistence {
  public class DataPersistenceManager {

    private static string sSaveFilePath = Application.persistentDataPath + "/SaveData.json";

    private static string sBackupSaveFilePath = sSaveFilePath + ".bak";

    public static Date Today {
      get {
        // Roll over at 4 am
        return DateTime.Now.AddHours(-4).Date;
      }
    }

    private DataPersistenceManager() {

      if (File.Exists(sSaveFilePath)) {
        try {
          string fileData = File.ReadAllText(sSaveFilePath);

          Data = JsonConvert.DeserializeObject<SaveData>(fileData, GlobalSerializerSettings.JsonSettings);
        }
        catch (Exception ex) {
          DAS.Error("DataPersistenceManager.Load", "Error Loading Saved Data: " + ex.Message);
          DAS.Debug("DataPersistenceManager.Load.StackTrace", DASUtil.FormatStackTrace(ex.StackTrace));
          HockeyApp.ReportStackTrace("DataPersistenceManager.Load", ex.StackTrace);

          // Try to load the backup file.
          try {
            string backupData = File.ReadAllText(sBackupSaveFilePath);
            Data = JsonConvert.DeserializeObject<SaveData>(backupData, GlobalSerializerSettings.JsonSettings);
          }
          catch (Exception ex2) {
            DAS.Error("DataPersistenceManager.LoadBackup", "Error Loading Backup Saved Data: " + ex2.Message);
            DAS.Debug("DataPersistenceManager.LoadBackup.StackTrace", DASUtil.FormatStackTrace(ex2.StackTrace));
            HockeyApp.ReportStackTrace("DataPersistenceManager.LoadBackup", ex2.StackTrace);
            Data = new SaveData();
          }
        }
      }
      else {
        DAS.Info(this, "Creating New Save Data");
        Data = new SaveData();
      }
    }

    // This is only called right after the save file is read in, and checks that the read in save file matches code.
    // if they don't a conversion is done.
    private void CheckSaveVersionUpdates() {
      try {
        PlayerProfile profile = Data.DefaultProfile;
        if (profile != null) {
          // In version 0 we let sessions grow infinitely in StartNewSession.
          // This chunk of code converts that into just storing a series of ints so the size is manageable
          if (profile.Sessions != null &&
              profile.SaveVersion == 0 && PlayerProfile.kSaveVersionCurrent > 0) {
            // This likely only happens when we're first switching over to the creation of total sessions
            if (profile.Sessions.Count > profile.TotalSessions) {
              profile.TotalSessions = profile.Sessions.Count;
            }

            foreach (TimelineEntryData sessionEntry in profile.Sessions) {
              if (sessionEntry.HasConnectedToCozmo) {
                profile.DaysWithCozmo++;
              }
            }

            if (profile.Sessions.LastOrDefault() == null) {
              profile.CurrentStreak = 0;
            }
            else if (profile.Sessions.Count < 2) {
              profile.CurrentStreak = 1;
            }
            else {
              int streakCount = 1;
              int currentIndex = profile.Sessions.Count - 1;
              int previousIndex = profile.Sessions.Count - 2;
              while (previousIndex >= 0) {
                Date currentDate = profile.Sessions[currentIndex].Date;
                Date previousDate = profile.Sessions[previousIndex].Date;
                if (previousDate.OffsetDays(1).Equals(currentDate)) {
                  streakCount++;
                }
                else {
                  break;
                }
                currentIndex--;
                previousIndex--;
              }
              profile.CurrentStreak = streakCount;
            }
          }
          // version 2: break out drone mode instructions seen from TotalGamesPlayed, err on the side of not showing again
          if (profile.SaveVersion < 2 && PlayerProfile.kSaveVersionCurrent >= 2) {
            int timesPlayedDroneMode;
            profile.TotalGamesPlayed.TryGetValue("DroneModeGame", out timesPlayedDroneMode);
            profile.DroneModeInstructionsSeen = timesPlayedDroneMode > 0;
          }
          profile.SaveVersion = PlayerProfile.kSaveVersionCurrent;
        }
      }
      catch (Exception ex) {
        // This error happens before DAS is initialized so the unity error messages makes sure we log the error to hockeyapp
        // but doesn't cause execution to stop and hang on the loading screen.
        Debug.LogError("DataPersistenceManager.CheckSaveVersionUpdates.Error, " + ex.StackTrace);
      }
    }

    private static DataPersistenceManager _Instance;

    public static DataPersistenceManager Instance {
      get {
        if (_Instance == null) {
          _Instance = new DataPersistenceManager();

          _Instance.CheckSaveVersionUpdates();
        }
        return _Instance;
      }
    }

    public static void CreateInstance() {
      if (_Instance == null) {
        _Instance = new DataPersistenceManager();
      }
    }

    public readonly SaveData Data;

    // Helper function to clean up code that checks for SDK mode
    public bool IsSDKEnabled {
      get {
        return Data.DeviceSettings.IsSDKEnabled;
      }
      set {
        Data.DeviceSettings.IsSDKEnabled = value;
      }
    }

    public bool IsNewSessionNeeded {
      get {
        var lastSession = Data.DefaultProfile.Sessions.LastOrDefault();
        return (lastSession == null || lastSession.Date != DataPersistenceManager.Today);
      }
    }

    public TimelineEntryData CurrentSession {
      get {
        var lastSession = Data.DefaultProfile.Sessions.LastOrDefault();
        if (lastSession != null && lastSession.Date == DataPersistenceManager.Today) {
          return lastSession;
        }
        return null;
      }
    }

    public int DisplayedSparks {
      get {
        return Data.DefaultProfile.DisplayedSparks;
      }
      set {
        Data.DefaultProfile.DisplayedSparks = value;
        Save();
      }
    }

    public int DisplayedStars {
      get {
        return Data.DefaultProfile.DisplayedStars;
      }
      set {
        Data.DefaultProfile.DisplayedStars = value;
        Save();
      }
    }

    public bool StarLevelToDisplay {
      get {
        return Data.DefaultProfile.NewStarLevels != null && Data.DefaultProfile.NewStarLevels.Count > 0;
      }
    }

    public void ClearNewStarLevels() {
      Data.DefaultProfile.NewStarLevels.Clear();
      Save();
    }

    public void AddNewStarLevel(Anki.Cozmo.ExternalInterface.StarLevelCompleted message) {
      Data.DefaultProfile.NewStarLevels.Add(message);
      Save();
    }

    public NeedsReward[] GetNeedsRewardsFromNewStarLevels() {
      List<NeedsReward> rewards = new List<NeedsReward>();
      foreach (Anki.Cozmo.ExternalInterface.StarLevelCompleted lvl in Data.DefaultProfile.NewStarLevels) {
        rewards.AddRange(lvl.rewards);
      }
      return rewards.ToArray();
    }

    public void SetHasConnectedWithCozmo(bool connected = true) {
      if (DataPersistenceManager.Instance.CurrentSession != null) {
        if (!DataPersistenceManager.Instance.CurrentSession.HasConnectedToCozmo) {
          DataPersistenceManager.Instance.Data.DefaultProfile.DaysWithCozmo++;
        }
        DataPersistenceManager.Instance.CurrentSession.HasConnectedToCozmo = connected;
      }
    }

    public string DeviceId { get; private set; }

    public TimelineEntryData StartNewSession() {
      TimelineEntryData newSession = new TimelineEntryData(DataPersistenceManager.Today);
      int numDays = 0;
      TimelineEntryData lastSession = null;
      PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
      if (DataPersistenceManager.Instance.Data.DefaultProfile.Sessions != null) {
        numDays = DataPersistenceManager.Instance.Data.DefaultProfile.TotalSessions;
        if (numDays > 0) {
          lastSession = profile.Sessions[profile.Sessions.Count - 1];
        }
      }

      profile.Sessions.Add(newSession);
      profile.TotalSessions++;

      // update streaks...
      if (lastSession != null) {
        Date currentDate = newSession.Date;
        Date previousDate = lastSession.Date;
        if (previousDate.OffsetDays(1).Equals(currentDate)) {
          profile.CurrentStreak++;
        }
        else {
          profile.CurrentStreak = 1;
        }
      }
      else {
        profile.CurrentStreak = 1;
      }

      const int kMaxDaysStored = 3;
      // Pop off sessions we're not going to need anymore. We still look at a few recent sessions for different daily goal gen.
      if (profile.Sessions.Count > kMaxDaysStored) {
        profile.Sessions.RemoveRange(0, profile.Sessions.Count - kMaxDaysStored);
      }

      if (Data.DefaultProfile.CurrentStreak >= Data.DefaultProfile.MaximumStreak) {
        Data.DefaultProfile.MaximumStreak = Data.DefaultProfile.CurrentStreak;
      }

      GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnNewDayStarted, Data.DefaultProfile.CurrentStreak));
      DataPersistenceManager.Instance.Save();

      return newSession;
    }


    public void Save() {
      try {
        if (File.Exists(sSaveFilePath)) {
          if (File.Exists(sBackupSaveFilePath)) {
            File.Delete(sBackupSaveFilePath);
          }

          File.Copy(sSaveFilePath, sBackupSaveFilePath);
        }

        string jsonValue = JsonConvert.SerializeObject(Data, Formatting.None, GlobalSerializerSettings.JsonSettings);
        File.WriteAllText(sSaveFilePath, jsonValue);
      }
      catch (Exception ex) {
        DAS.Error("DataPersistenceManager.Save", "Exception backing up save file: " + ex.Message);
        DAS.Debug("DataPersistenceManager.Save.StackTrace", DASUtil.FormatStackTrace(ex.StackTrace));
        HockeyApp.ReportStackTrace("DataPersistenceManager.Save", ex.StackTrace);
      }
    }

    #region DebugMenuAPI

    public string GetSaveJSON() {
      return JsonConvert.SerializeObject(Data, Formatting.None, GlobalSerializerSettings.JsonSettings);
    }

    public void HandleSupportInfo(Anki.Cozmo.ExternalInterface.SupportInfo info) {
      DeviceId = info.deviceId;
    }

    public void HandleUpdatedAssignments(Anki.Cozmo.ExternalInterface.UpdatedAssignments message) {
      Data.DefaultProfile.LabAssignments = message.assignments;
      Save();
    }

    #endregion



  }
}


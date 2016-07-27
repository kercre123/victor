using System;
using UnityEngine;
using System.IO;
using Newtonsoft.Json;
using System.Collections.Generic;
using Cozmo.Util;

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
          DAS.Error(this, "Error Loading Saved Data: " + ex);

          // Try to load the backup file.
          try {
            string backupData = File.ReadAllText(sBackupSaveFilePath);

            Data = JsonConvert.DeserializeObject<SaveData>(backupData, GlobalSerializerSettings.JsonSettings);
          }
          catch (Exception ex2) {
            DAS.Error(this, "Error Loading Backup Saved Data: " + ex2);

            Data = new SaveData();
          }
        }
      }
      else {
        DAS.Info(this, "Creating New Save Data");
        Data = new SaveData();
      }
    }

    private static DataPersistenceManager _Instance;

    public static DataPersistenceManager Instance {
      get {
        if (_Instance == null) {
          _Instance = new DataPersistenceManager();
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

    public bool IsFirstSession {
      get {
        return (Data.DefaultProfile.Sessions.LastOrDefault() == null);
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

    public TimelineEntryData StartNewSession() {
      // create a new session
      // If we have a previous session, check to see if we have a streak going,
      // reward appropriate rewards for the streak otherwise reset the streak
      if (Data.DefaultProfile.Sessions.LastOrDefault() != null) {
        Date Yesterday = DataPersistenceManager.Today.AddDays(-1);
        if (Data.DefaultProfile.Sessions.LastOrDefault().Date.Equals(Yesterday)) {
          DataPersistenceManager.Instance.Data.DefaultProfile.CurrentStreak++;
          // Reward Daily Check In Streak rewards to inventory, keep track of them
          // for the CheckInFlow to display
          // TODO : Create Equivalent of ChestRewardManager for StreakRewards
        }
      }
      else {
        DataPersistenceManager.Instance.Data.DefaultProfile.CurrentStreak = 0;
      }
      TimelineEntryData newSession = new TimelineEntryData(DataPersistenceManager.Today);
      newSession.DailyGoals = DailyGoalManager.Instance.GenerateDailyGoals();
      // Sort by priority, placing higher priority at the front of the list
      newSession.DailyGoals.Sort((Cozmo.UI.DailyGoal x, Cozmo.UI.DailyGoal y) => {
        return y.Priority.CompareTo(x.Priority);
      });
      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Add(newSession);
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
      }
      catch (Exception ex) {
        DAS.Error(this, "Exception backing up save file: " + ex);
      }

      string jsonValue = JsonConvert.SerializeObject(Data, Formatting.None, GlobalSerializerSettings.JsonSettings);
      File.WriteAllText(sSaveFilePath, jsonValue);
    }

    #region DebugMenuAPI

    public string GetSaveJSON() {
      return JsonConvert.SerializeObject(Data, Formatting.None, GlobalSerializerSettings.JsonSettings);
    }

    public void DebugSave(string jsonValue) {
      try {
        if (File.Exists(sSaveFilePath)) {
          if (File.Exists(sBackupSaveFilePath)) {
            File.Delete(sBackupSaveFilePath);
          }

          File.Copy(sSaveFilePath, sBackupSaveFilePath);
        }
      }
      catch (Exception ex) {
        DAS.Error(this, "Exception backing up save file: " + ex);
      }

      File.WriteAllText(sSaveFilePath, jsonValue);
    }

    #endregion



  }
}


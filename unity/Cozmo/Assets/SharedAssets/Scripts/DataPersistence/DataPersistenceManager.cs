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

    public int CurrentStreak {
      get {
        if (Data.DefaultProfile.Sessions.LastOrDefault() == null) {
          return 0;
        }
        if (Data.DefaultProfile.Sessions.Count < 2) {
          return 1;
        }

        int streakCount = 1;

        int currentIndex = Data.DefaultProfile.Sessions.Count - 1;
        int previousIndex = Data.DefaultProfile.Sessions.Count - 2;

        while (previousIndex >= 0) {
          Date currentDate = Data.DefaultProfile.Sessions[currentIndex].Date;
          Date previousDate = Data.DefaultProfile.Sessions[previousIndex].Date;

          if (previousDate.AddDays(1).Equals(currentDate)) {
            streakCount++;
          }
          else {
            return streakCount;
          }

          currentIndex--;
          previousIndex--;
        }

        return streakCount;
      }
    }

    public string DeviceId { get; private set; }

    public TimelineEntryData StartNewSession() {
      TimelineEntryData newSession = new TimelineEntryData(DataPersistenceManager.Today);
      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Add(newSession);
      // Goals have been generated for onboarding, complete this phase so that future sessions will have fresh goals
      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.DailyGoals)) {
        OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.DailyGoals);
      }

      newSession.DailyGoals = DailyGoalManager.Instance.GenerateDailyGoals();
      if (DailyGoalManager.Instance.OnRefreshDailyGoals != null) {
        DailyGoalManager.Instance.OnRefreshDailyGoals.Invoke();
      }

      if (CurrentStreak > Data.DefaultProfile.MaximumStreak) {
        Data.DefaultProfile.MaximumStreak = CurrentStreak;
      }
      GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnNewDayStarted, CurrentStreak));
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

    public void HandleSupportInfo(Anki.Cozmo.ExternalInterface.SupportInfo info) {
      DeviceId = info.deviceId;
    }

    #endregion



  }
}


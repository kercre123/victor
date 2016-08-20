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
        int streak = 0;
        bool streakBroken = false;
        List<TimelineEntryData> allSessions = new List<TimelineEntryData>();
        allSessions.AddRange(Data.DefaultProfile.Sessions);
        allSessions.Reverse();
        Date CurrDay = allSessions[0].Date;
        Date PrevDay = allSessions[1].Date;
        while (streakBroken == false) {
          if (CurrDay.AddDays(-1).Equals(PrevDay)) {
            streak += 1;
            // Remove Current Day, set to prev Day
            CurrDay = PrevDay;
            allSessions.RemoveAt(0);
            if (allSessions.Count < 2) {
              return streak;
            }
            // Get next PrevDay
            PrevDay = allSessions[1].Date;
          }
          else {
            streakBroken = true;
          }
        }
        return streak;
      }
    }

    public TimelineEntryData StartNewSession() {
      TimelineEntryData newSession = new TimelineEntryData(DataPersistenceManager.Today);
      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Add(newSession);

      bool needsNewGoals = true;
      // Onboarding special case, keep repeating goals until completed.
      if (Data.DefaultProfile.Sessions != null &&
          Data.DefaultProfile.Sessions.Count > 0 &&
          OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.DailyGoals)) {

        // Copy the previous days goals and just put in new dates...
        TimelineEntryData currSession = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault();
        // this is mostly because people debug mess with their save a lot in onboarding.
        // In the normal case this phase is completed with a DailyGoalCompleted event.
        bool allGoalsComplete = currSession.DailyGoals.TrueForAll(x => x.GoalComplete);
        if (allGoalsComplete) {
          OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.DailyGoals);
        }
        else {
          needsNewGoals = false;
          newSession.DailyGoals.AddRange(currSession.DailyGoals);
        }
      }

      if (needsNewGoals) {
        newSession.DailyGoals = DailyGoalManager.Instance.GenerateDailyGoals();
        // Sort by priority, placing higher priority at the front of the list
        newSession.DailyGoals.Sort((Cozmo.UI.DailyGoal x, Cozmo.UI.DailyGoal y) => {
          return y.Priority.CompareTo(x.Priority);
        });

        if (DailyGoalManager.Instance.OnRefreshDailyGoals != null) {
          DailyGoalManager.Instance.OnRefreshDailyGoals.Invoke();
        }
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

    #endregion



  }
}


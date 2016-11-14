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

          if (previousDate.OffsetDays(1).Equals(currentDate)) {
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
      int numDays = 0;
      TimelineEntryData lastSession = null;
      if (DataPersistenceManager.Instance.Data.DefaultProfile.Sessions != null) {
        numDays = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Count;
        if (numDays > 0) {
          lastSession = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions[numDays - 1];
          // If this is not the first day, consider onboarding complete so we generate new goals
          if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.DailyGoals)) {
            OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.DailyGoals);
          }
        }
      }
      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Add(newSession);

      if (lastSession != null && !lastSession.GoalsFinished) {
        Cozmo.UI.DailyGoal goal = lastSession.DailyGoals.FirstOrDefault();
        string goalName = "";
        if (goal != null) {
          goalName = goal.Title.Key;
        }
        DAS.Event("meta.goal_expired", numDays.ToString(), DASUtil.FormatExtraData(goalName));
        foreach (Cozmo.UI.DailyGoal dailyGoal in lastSession.DailyGoals) {
          DAS.Event("meta.goal_expired_progress", dailyGoal.Title.Key, DASUtil.FormatExtraData(
            string.Format("{0}/{1}", dailyGoal.Progress, dailyGoal.Target)));
        }

      }

      newSession.DailyGoals = DailyGoalManager.Instance.GenerateDailyGoals();
      // log daily goals
      foreach (Cozmo.UI.DailyGoal dailyGoal in newSession.DailyGoals) {
        DAS.Event("meta.daily_goals", numDays.ToString(), DASUtil.FormatExtraData(dailyGoal.Title.Key));
      }
      if (DailyGoalManager.Instance.OnRefreshDailyGoals != null) {
        DailyGoalManager.Instance.OnRefreshDailyGoals.Invoke();
      }

      if (CurrentStreak > Data.DefaultProfile.MaximumStreak) {
        Data.DefaultProfile.MaximumStreak = CurrentStreak;
      }
      if (!OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Home)) {
        GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnNewDayStarted, CurrentStreak));
      }
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
        DAS.Error(this, "Exception backing up save file: " + ex);
      }
    }

    #region DebugMenuAPI

    public string GetSaveJSON() {
      return JsonConvert.SerializeObject(Data, Formatting.None, GlobalSerializerSettings.JsonSettings);
    }

    public void HandleSupportInfo(Anki.Cozmo.ExternalInterface.SupportInfo info) {
      DeviceId = info.deviceId;
    }

    #endregion



  }
}


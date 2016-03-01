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

    public readonly SaveData Data;

    public TimelineEntryData CurrentSession { 
      get { 
        var lastSession = Data.Sessions.LastOrDefault();
        if (lastSession != null && lastSession.Date == DataPersistenceManager.Today) {            
          return lastSession;
        }
        return null;
      } 
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
  }
}


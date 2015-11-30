using System;
using UnityEngine;
using System.IO;
using Newtonsoft.Json;
using System.Collections.Generic;

namespace DataPersistence {
  public class DataPersistenceManager {

    private static string sSaveFilePath = Application.persistentDataPath + "/SaveData.json";

    private static string sBackupSaveFilePath = sSaveFilePath + ".bak";

    private static JsonSerializerSettings _JsonSettings;

    public static JsonSerializerSettings JsonSettings {
      get {
        if (_JsonSettings == null) {
          _JsonSettings = new JsonSerializerSettings() {
            TypeNameHandling = TypeNameHandling.Auto,
            Converters = new List<JsonConverter> {
              new UtcDateTimeConverter(),
              new Newtonsoft.Json.Converters.StringEnumConverter()
            }
          };
        }
        return _JsonSettings;
      }
    }

    private DataPersistenceManager() { 
      if (File.Exists(sSaveFilePath)) {
        try {
          string fileData = File.ReadAllText(sSaveFilePath);

          Data = JsonConvert.DeserializeObject<SaveData>(fileData, JsonSettings);
        }
        catch (Exception ex) {
          DAS.Error(this, "Error Loading Saved Data: " + ex);

          // Try to load the backup file.
          try {
            string backupData = File.ReadAllText(sBackupSaveFilePath);

            Data = JsonConvert.DeserializeObject<SaveData>(backupData, JsonSettings);
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

    public void Save() {
      File.Copy(sSaveFilePath, sBackupSaveFilePath);

      string jsonValue = JsonConvert.SerializeObject(Data, Formatting.None, JsonSettings);

      File.WriteAllText(sSaveFilePath, jsonValue);
    }
  }
}


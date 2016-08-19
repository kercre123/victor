using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DataPersistence {
  public class SaveData {
    public PlayerProfile DefaultProfile;
    public DebugProfile DebugPrefs;
    public DeviceProfile DeviceSettings;

    public SaveData() {
      DefaultProfile = new PlayerProfile();
      DebugPrefs = new DebugProfile();
      DeviceSettings = new DeviceProfile();
    }
  }
}
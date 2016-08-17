using System.Collections.Generic;

namespace DataPersistence {
  [System.Serializable]
  public class DeviceProfile {
    public bool IsSDKEnabled;
    public Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float> VolumePreferences;

    public DeviceProfile() {
      IsSDKEnabled = false;
      VolumePreferences = new Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float>();
    }
  }
}
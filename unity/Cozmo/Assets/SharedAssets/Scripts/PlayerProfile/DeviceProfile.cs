using System.Collections.Generic;

namespace DataPersistence {
  [System.Serializable]
  public class DeviceProfile {
    public bool IsSDKEnabled;
    public bool SDKActivated;
    public Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float> VolumePreferences;

    public DeviceProfile() {
      IsSDKEnabled = false;
      SDKActivated = false;
      VolumePreferences = new Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float>();
    }
  }
}
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Anki.Cozmo.Audio {
  public class AudioMixerPane : MonoBehaviour {

    [SerializeField]
    private RectTransform _MixerTray;

    [SerializeField]
    private MixerStrip _MixerStripPrefab;

    private float _kDefaultVolume = 0.7f;

    private void Awake() {

      if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.RunPressDemo) {
        _kDefaultVolume = 1.0f;
      }

      var types = System.Enum.GetValues(typeof(VolumeParameters.VolumeType));
      Dictionary<VolumeParameters.VolumeType, float> currentVolumePrefs = DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.VolumePreferences;
      bool prefsChanged = false;
      foreach (var type in types) {
        var volumeType = (VolumeParameters.VolumeType)type;

        var mixerStrip = UIManager.CreateUIElement(_MixerStripPrefab, _MixerTray).GetComponent<MixerStrip>();

        mixerStrip.VolumeType = volumeType;
        float volume;
        if (!currentVolumePrefs.TryGetValue(volumeType, out volume)) {
          volume = _kDefaultVolume;
          currentVolumePrefs[volumeType] = volume;
          prefsChanged = true;
        }
        mixerStrip.Value = volume;
        mixerStrip.OnValueChange += HandleValueChanged;
      }
      if (prefsChanged) {
        DataPersistence.DataPersistenceManager.Instance.Save();
      }
    }

    private void HandleValueChanged(VolumeParameters.VolumeType volumeType, float volume) {
      GameAudioClient.SetVolumeValue(volumeType, volume);
    }

  }
}
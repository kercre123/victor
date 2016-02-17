using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Anki.Cozmo.Audio {
  public class AudioMixerPane : MonoBehaviour {

    [SerializeField]
    private RectTransform _MixerTray;

    [SerializeField]
    private MixerStrip _MixerStripPrefab;

    /// <summary>
    /// TODO: Track this in our audio system somewhere
    /// </summary>
    private static readonly Dictionary<VolumeParameters.VolumeType, float> _CurrentVolumes = new Dictionary<VolumeParameters.VolumeType, float>();

    private void Awake() {
      var types = System.Enum.GetValues(typeof(VolumeParameters.VolumeType));
      foreach(var type in types) {
        var volumeType = (VolumeParameters.VolumeType)type;

        var mixerStrip = UIManager.CreateUIElement(_MixerStripPrefab, _MixerTray).GetComponent<MixerStrip>();

        mixerStrip.VolumeType = volumeType;
        float volume;
        if (!_CurrentVolumes.TryGetValue(volumeType, out volume)) {
          volume = 1f;
          _CurrentVolumes[volumeType] = volume;
        }
        mixerStrip.Value = volume;
        mixerStrip.OnValueChange += HandleValueChanged;
      }
    }

    private void HandleValueChanged(VolumeParameters.VolumeType volumeType, float volume) {
      _CurrentVolumes[volumeType] = volume;
      GameAudioClient.SetVolumeValue(volumeType, volume);
    }

  }
}
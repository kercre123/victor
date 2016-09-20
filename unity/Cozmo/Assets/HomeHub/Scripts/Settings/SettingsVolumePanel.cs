using Anki.Cozmo.Audio;
using Anki.Cozmo.Audio.VolumeParameters;
using Cozmo.UI;
using DataPersistence;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SettingsVolumePanel : MonoBehaviour {
    [SerializeField]
    private CozmoToggleButton _LowRobotVolumeToggle;

    [SerializeField]
    private CozmoToggleButton _MediumRobotVolumeToggle;

    [SerializeField]
    private CozmoToggleButton _HighRobotVolumeToggle;

    // Music volume slider
    [SerializeField]
    private Slider _DeviceVolumeSlider;

    private void Start() {
      Dictionary<VolumeType, float> currentVolumePrefs = DataPersistenceManager.Instance.Data.DeviceSettings.VolumePreferences;

      float robotVolume = 0.0f;
      if (!currentVolumePrefs.TryGetValue(VolumeType.Robot, out robotVolume)) {
        robotVolume = DefaultSettingsValuesConfig.Instance.DefaultRobotVolumeLevelValue;
      }

      float musicVolume = 0.0f;
      if (!currentVolumePrefs.TryGetValue(VolumeType.Music, out musicVolume)) {
        musicVolume = DefaultSettingsValuesConfig.Instance.DefaultMusicVolumeLevel;
      }
      _DeviceVolumeSlider.value = musicVolume;
      _DeviceVolumeSlider.onValueChanged.AddListener(HandleMusicVolumeValueChanged);

      _LowRobotVolumeToggle.Initialize(HandleLowRobotVolumeTogglePressed, "settings_low_robot_volume", "settings_panel");
      _MediumRobotVolumeToggle.Initialize(HandleMediumRobotVolumeTogglePressed, "settings_medium_robot_volume", "settings_panel");
      _HighRobotVolumeToggle.Initialize(HandleHighRobotVolumeTogglePressed, "settings_high_robot_volume", "settings_panel");

      if (robotVolume <= DefaultSettingsValuesConfig.Instance.LowRobotVolume) {
        _LowRobotVolumeToggle.ShowPressedStateOnRelease = true;
      }
      else if (robotVolume <= DefaultSettingsValuesConfig.Instance.MediumRobotVolume) {
        _MediumRobotVolumeToggle.ShowPressedStateOnRelease = true;
      }
      else {
        _HighRobotVolumeToggle.ShowPressedStateOnRelease = true;
      }
    }

    private void HandleMusicVolumeValueChanged(float volume) {
      GameAudioClient.SetVolumeValue(VolumeType.Music, volume);
    }

    private void HandleLowRobotVolumeTogglePressed() {
      _LowRobotVolumeToggle.ShowPressedStateOnRelease = true;
      _MediumRobotVolumeToggle.ShowPressedStateOnRelease = false;
      _HighRobotVolumeToggle.ShowPressedStateOnRelease = false;
      GameAudioClient.SetVolumeValue(VolumeType.Robot, DefaultSettingsValuesConfig.Instance.LowRobotVolume);
    }

    private void HandleMediumRobotVolumeTogglePressed() {
      _LowRobotVolumeToggle.ShowPressedStateOnRelease = false;
      _MediumRobotVolumeToggle.ShowPressedStateOnRelease = true;
      _HighRobotVolumeToggle.ShowPressedStateOnRelease = false;
      GameAudioClient.SetVolumeValue(VolumeType.Robot, DefaultSettingsValuesConfig.Instance.MediumRobotVolume);
    }

    private void HandleHighRobotVolumeTogglePressed() {
      _LowRobotVolumeToggle.ShowPressedStateOnRelease = false;
      _MediumRobotVolumeToggle.ShowPressedStateOnRelease = false;
      _HighRobotVolumeToggle.ShowPressedStateOnRelease = true;
      GameAudioClient.SetVolumeValue(VolumeType.Robot, DefaultSettingsValuesConfig.Instance.HighRobotVolume);
    }
  }
}

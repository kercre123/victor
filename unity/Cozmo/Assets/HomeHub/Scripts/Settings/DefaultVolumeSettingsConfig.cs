using UnityEngine;
using System.Collections;

namespace Cozmo.Settings {
  public class DefaultVolumeSettingsConfig : ScriptableObject {
    public enum RobotVolumeLevels {
      LOW,
      MEDIUM,
      HIGH
    }

    private static DefaultVolumeSettingsConfig _sInstance;
    public static DefaultVolumeSettingsConfig Instance { get { return _sInstance; } }

    public static void SetInstance(DefaultVolumeSettingsConfig instance) {
      _sInstance = instance;
    }

    [SerializeField, Range(0f, 1f)]
    private float _LowRobotVolume = 0.2f;
    public float LowRobotVolume { get { return _LowRobotVolume; } }

    [SerializeField, Range(0f, 1f)]
    private float _MediumRobotVolume = 0.6f;
    public float MediumRobotVolume { get { return _MediumRobotVolume; } }

    [SerializeField, Range(0f, 1f)]
    private float _HighRobotVolume = 1f;
    public float HighRobotVolume { get { return _HighRobotVolume; } }

    private RobotVolumeLevels _DefaultRobotVolumeLevel = RobotVolumeLevels.HIGH;
    public RobotVolumeLevels DefaultRobotVolumeLevel { get { return _DefaultRobotVolumeLevel; } }

    [SerializeField, Range(0f, 1f)]
    private float _DefaultMasterVolume = 1f;
    public float DefaultMasterVolumeLevel { get { return _DefaultMasterVolume; } }
  }
}
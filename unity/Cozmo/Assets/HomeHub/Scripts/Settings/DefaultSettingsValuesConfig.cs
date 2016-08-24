using UnityEngine;
using System.Collections;

namespace Cozmo.Settings {
  public class DefaultSettingsValuesConfig : ScriptableObject {
    public enum RobotVolumeLevels {
      LOW,
      MEDIUM,
      HIGH
    }

    private static DefaultSettingsValuesConfig _sInstance;
    public static DefaultSettingsValuesConfig Instance { get { return _sInstance; } }

    public static void SetInstance(DefaultSettingsValuesConfig instance) {
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

    [SerializeField]
    private string _SdkUrl = "https://developer.anki.com/en-us";
    public string SdkUrl { get { return _SdkUrl; } }

    [SerializeField, Range(-1f, 120f)]
    public float _AppBackground_TimeTilSleep_sec = 20f;
    public float AppBackground_TimeTilSleep_sec { get { return _AppBackground_TimeTilSleep_sec; } }

    [SerializeField, Range(-1f, 120f)]
    public float _AppBackground_TimeTilDisconnect_sec = 40f;
    public float AppBackground_TimeTilDisconnect_sec { get { return _AppBackground_TimeTilDisconnect_sec; } }

    [SerializeField, Range(-1f, 120f)]
    public float _PlayerSleepCozmo_TimeTilSleep_sec = 0f;
    public float PlayerSleepCozmo_TimeTilSleep_sec { get { return _PlayerSleepCozmo_TimeTilSleep_sec; } }

    [SerializeField, Range(-1f, 120f)]
    public float _PlayerSleepCozmo_TimeTilDisconnect_sec = 2f;
    public float PlayerSleepCozmo_TimeTilDisconnect_sec { get { return _PlayerSleepCozmo_TimeTilDisconnect_sec; } }

    [SerializeField, Range(0f, 1f)]
    private float _FilterSmoothingWeight = 0.99f;
    public float FilterSmoothingWeight { get { return _FilterSmoothingWeight; } }

    [SerializeField, Range(3f, 4.2f)]
    private float _LowBatteryVoltageValue = 3.5f;
    public float LowBatteryVoltageValue { get { return _LowBatteryVoltageValue; } }
  }
}
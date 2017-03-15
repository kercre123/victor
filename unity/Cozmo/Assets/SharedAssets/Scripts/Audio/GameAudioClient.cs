using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;


namespace Anki {
  namespace Cozmo {
    namespace Audio {

      namespace VolumeParameters {
        public enum VolumeType : uint {
          UI = AudioMetaData.GameParameter.ParameterType.Ui_Volume,
          SFX = AudioMetaData.GameParameter.ParameterType.Sfx_Volume,
          VO = AudioMetaData.GameParameter.ParameterType.Vo_Volume,
          Music = AudioMetaData.GameParameter.ParameterType.Music_Volume,
          Robot = AudioMetaData.GameParameter.ParameterType.Robot_Volume
        }
      }

      [System.Serializable]
      public struct AudioEventParameter {

        public static AudioEventParameter InvalidEvent = new AudioEventParameter(AudioMetaData.GameEvent.GenericEvent.Invalid, 
                                                                                 AudioMetaData.GameEvent.EventGroupType.GenericEvent);

        public static AudioEventParameter DefaultClick = new AudioEventParameter(AudioMetaData.GameEvent.GenericEvent.Play__Ui__Click_General,
                                                                                 AudioMetaData.GameEvent.EventGroupType.Ui);

        public static AudioEventParameter UIEvent(AudioMetaData.GameEvent.Ui ui) {
          return new AudioEventParameter((AudioMetaData.GameEvent.GenericEvent)ui, AudioMetaData.GameEvent.EventGroupType.Ui);
        }

        public static AudioEventParameter SFXEvent(AudioMetaData.GameEvent.Sfx sfx) {
          return new AudioEventParameter((AudioMetaData.GameEvent.GenericEvent)sfx, AudioMetaData.GameEvent.EventGroupType.Sfx);
        }

        // Unity doesn't like uints for some reason
        [SerializeField]
        private string _Event;

        [SerializeField]
        private string _EventType;

        private AudioEventParameter(AudioMetaData.GameEvent.GenericEvent evt, AudioMetaData.GameEvent.EventGroupType evtType) {
          _Event = evt.ToString();
          _EventType = evtType.ToString();
        }

        public AudioMetaData.GameEvent.GenericEvent Event {
          get {
            // if we used the default, return invalid
            if (string.IsNullOrEmpty(_Event)) {
              return AudioMetaData.GameEvent.GenericEvent.Invalid;
            }

            return (AudioMetaData.GameEvent.GenericEvent)Enum.Parse(typeof(AudioMetaData.GameEvent.GenericEvent), _Event);
          }
          set {
            _Event = value.ToString();
          }
        }

        public AudioMetaData.GameEvent.EventGroupType EventType {
          get {
            // if we used the default, return generic
            if (string.IsNullOrEmpty(_EventType)) {
              return AudioMetaData.GameEvent.EventGroupType.GenericEvent;
            }

            return (AudioMetaData.GameEvent.EventGroupType)Enum.Parse(typeof(AudioMetaData.GameEvent.EventGroupType), _EventType);
          }
          set {
            _EventType = value.ToString();
          }
        }

        public AudioMetaData.GameObjectType GetGameObjectType() {
          var eventType = EventType;

          switch (eventType) {
          case AudioMetaData.GameEvent.EventGroupType.Sfx:
            return AudioMetaData.GameObjectType.SFX;

          case AudioMetaData.GameEvent.EventGroupType.Ui:
            return AudioMetaData.GameObjectType.UI;

          // Only SFX and UI are valid for the time being
          case AudioMetaData.GameEvent.EventGroupType.Coz_App:
          case AudioMetaData.GameEvent.EventGroupType.Dev_Device:
          case AudioMetaData.GameEvent.EventGroupType.Dev_Robot:
          case AudioMetaData.GameEvent.EventGroupType.GenericEvent:
          case AudioMetaData.GameEvent.EventGroupType.Music:
          case AudioMetaData.GameEvent.EventGroupType.Robot_Sfx:
          case AudioMetaData.GameEvent.EventGroupType.Robot_Vo:
          default:
            return AudioMetaData.GameObjectType.Invalid;
          }
        }

        public bool IsInvalid() {
          return AudioMetaData.GameObjectType.Invalid == GetGameObjectType();
        }
      }

      public class GameAudioClient {

        // If you want to listen to all audio callback events register with Audio Client
        //    AudioClient client = AudioClient.Instance;
        //    client.OnAudioCallback += YourHandler

        static public ushort PostAudioEvent(AudioEventParameter parameter,
                                            AudioEngine.Multiplexer.AudioCallbackFlag callbackFlag = AudioEngine.Multiplexer.AudioCallbackFlag.EventNone,
                                            CallbackHandler handler = null) {
          UnityAudioClient client = UnityAudioClient.Instance;
          return client.PostEvent(parameter.Event, parameter.GetGameObjectType(), callbackFlag, handler);
        }

        static public ushort PostUIEvent(AudioMetaData.GameEvent.Ui audioEvent,
                                         AudioEngine.Multiplexer.AudioCallbackFlag callbackFlag = AudioEngine.Multiplexer.AudioCallbackFlag.EventNone,
                                         CallbackHandler handler = null) {
          UnityAudioClient client = UnityAudioClient.Instance;
          return client.PostEvent((AudioMetaData.GameEvent.GenericEvent)audioEvent, AudioMetaData.GameObjectType.UI, callbackFlag, handler);
        }

        static public ushort PostSFXEvent(AudioMetaData.GameEvent.Sfx audioEvent,
                                          AudioEngine.Multiplexer.AudioCallbackFlag callbackFlag = AudioEngine.Multiplexer.AudioCallbackFlag.EventNone,
                                          CallbackHandler handler = null) {
          UnityAudioClient client = UnityAudioClient.Instance;
          return client.PostEvent((AudioMetaData.GameEvent.GenericEvent)audioEvent, AudioMetaData.GameObjectType.SFX, callbackFlag, handler);
        }

        static public ushort PostAnnouncerVOEvent(AudioMetaData.GameEvent.GenericEvent audioEvent,
                                                  AudioEngine.Multiplexer.AudioCallbackFlag callbackFlag = AudioEngine.Multiplexer.AudioCallbackFlag.EventNone,
                                                  CallbackHandler handler = null) {
          UnityAudioClient client = UnityAudioClient.Instance;
          return client.PostEvent(audioEvent, AudioMetaData.GameObjectType.Aria, callbackFlag, handler);
        }

        // Remove callback handle from Audio Client
        static public void UnregisterCallbackHandler(ushort playId) {
          UnityAudioClient client = UnityAudioClient.Instance;
          client.UnregisterCallbackHandler(playId);
        }

        static public void SetVolumeValue(VolumeParameters.VolumeType parameter, float volume, int timeInMS = 0, AudioEngine.Multiplexer.CurveType curve = AudioEngine.Multiplexer.CurveType.Linear, bool storeValue = true) {
          // Must sent to the robot's volume through the robot object
          if (parameter == Anki.Cozmo.Audio.VolumeParameters.VolumeType.Robot) {
            IRobot robot = RobotEngineManager.Instance.CurrentRobot;
            if (robot != null) {
              robot.SetRobotVolume(volume);
            }
            else {
              DAS.Warn("GameAudioClient.SetVolumeValue", "Attempt to VolumeParameters.VolumeType.Robot value, Robot is NULL");
            }
          }
          else {
            // User GameObjectType.Invalid to set global RTPC values
            UnityAudioClient client = UnityAudioClient.Instance;
            client.PostParameter((AudioMetaData.GameParameter.ParameterType)parameter, volume, AudioMetaData.GameObjectType.Invalid, timeInMS, curve);
          }

          if (storeValue) {
            System.Collections.Generic.Dictionary<VolumeParameters.VolumeType, float> volumePrefs = DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.VolumePreferences;
            if (volumePrefs.ContainsKey(parameter)) {
              volumePrefs[parameter] = volume;
            }
            else {
              volumePrefs.Add(parameter, volume);
            }
            DataPersistence.DataPersistenceManager.Instance.Save();
          }
        }

        // Use stored values to set all the volume parmaeters
        static public void SetPersistenceVolumeValues(VolumeParameters.VolumeType[] volumeParams = null) {
          // If list isn't provided set all volume parameter types
          List<VolumeParameters.VolumeType> volumeParamList = null;
          if (volumeParams != null) {
            volumeParamList = new List<Anki.Cozmo.Audio.VolumeParameters.VolumeType>(volumeParams);
          }
          else {
            volumeParamList = Enum.GetValues(typeof(VolumeParameters.VolumeType)).Cast<VolumeParameters.VolumeType>().ToList();
          }

          // Get stored volume parameters
          Dictionary<VolumeParameters.VolumeType, float> volumePrefs = DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.VolumePreferences;
          // Set each parameters
          foreach (VolumeParameters.VolumeType aParameter in volumeParamList) {
            float aValue;
            bool hasVolumePref = volumePrefs.TryGetValue(aParameter, out aValue);
            if (!hasVolumePref) {
              aValue = GetDefaultVolume(aParameter);
            }
            SetVolumeValue(aParameter, aValue, 0, AudioEngine.Multiplexer.CurveType.Linear, false);
          }
        }

        // Get the stored or default volume for the volume type
        static public float GetVolume(VolumeParameters.VolumeType volType) {
          var volumePrefs = DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.VolumePreferences;
          float value;
          if (!volumePrefs.TryGetValue(volType, out value)) {
            value = GetDefaultVolume(volType);
          }
          return value;
        }

        // Define default volume values
        static public float GetDefaultVolume(VolumeParameters.VolumeType volType) {
          float value = 1.0f;
          switch (volType) {
          case VolumeParameters.VolumeType.Music:
            value = 0.8f;
            break;

          case VolumeParameters.VolumeType.Robot: {
#if UNITY_EDITOR
              value = 0.6f;
#else
              value = 1.0f;
#endif
            }
            break;

          case VolumeParameters.VolumeType.SFX:
            value = 1.0f;
            break;

          case VolumeParameters.VolumeType.UI:
            value = 1.0f;
            break;

          case VolumeParameters.VolumeType.VO:
            value = 1.0f;
            break;
          }
          return value;
        }

        // Set Music States
        static public void SetMusicState(AudioMetaData.GameState.Music state,
                                         bool interrupt = false,
                                         uint minDurationInMilliSeconds = 0) {
          UnityAudioClient client = UnityAudioClient.Instance;
          client.PostMusicState((AudioMetaData.GameState.GenericState)state, interrupt, minDurationInMilliSeconds);
        }

       
        // Game Helpers
        static public void SetMusicRoundState(int round) {
          AudioMetaData.SwitchState.Gameplay_Round roundState = AudioMetaData.SwitchState.Gameplay_Round.Invalid;
          switch (round) {
          case 1:
            roundState = AudioMetaData.SwitchState.Gameplay_Round.Round_01;
            break;
          case 2:
            roundState = AudioMetaData.SwitchState.Gameplay_Round.Round_02;
            break;
          case 3:
            roundState = AudioMetaData.SwitchState.Gameplay_Round.Round_03;
            break;
          case 4:
            roundState = AudioMetaData.SwitchState.Gameplay_Round.Round_04;
            break;
          case 5:
            roundState = AudioMetaData.SwitchState.Gameplay_Round.Round_05;
            break;
          default:
            // Set default state
            roundState = AudioMetaData.SwitchState.Gameplay_Round.Round_01;
            DAS.Error("GameAudioClient.SetMusicRoundState", string.Format("Unhandled round value: {0}", round));
            break;
          }

          UnityAudioClient client = UnityAudioClient.Instance;
          client.PostSwitchState(AudioMetaData.SwitchState.SwitchGroupType.Gameplay_Round, (AudioMetaData.SwitchState.GenericSwitch)roundState, AudioMetaData.GameObjectType.Default);
        }

        static public void SetSparkedMusicState(AudioMetaData.SwitchState.Sparked sparked) {
          UnityAudioClient client = UnityAudioClient.Instance;
          client.PostSwitchState(AudioMetaData.SwitchState.SwitchGroupType.Sparked, (AudioMetaData.SwitchState.GenericSwitch)sparked, AudioMetaData.GameObjectType.Default);
        }
     
      }
    }
  }
}

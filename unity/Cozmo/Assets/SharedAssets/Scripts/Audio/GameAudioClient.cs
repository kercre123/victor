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
          UI = GameParameter.ParameterType.Ui_Volume,
          SFX = GameParameter.ParameterType.Sfx_Volume,
          VO = GameParameter.ParameterType.Vo_Volume,
          Music = GameParameter.ParameterType.Music_Volume,
          Robot = GameParameter.ParameterType.Robot_Volume
        }
      }

      [System.Serializable]
      public struct AudioEventParameter {

        public static AudioEventParameter InvalidEvent = new AudioEventParameter(GameEvent.GenericEvent.Invalid, 
                                                                                 GameEvent.EventGroupType.GenericEvent);

        public static AudioEventParameter DefaultClick = new AudioEventParameter(
                                                           GameEvent.GenericEvent.Play__Ui__Click_General,
                                                           GameEvent.EventGroupType.Ui);

        public static AudioEventParameter UIEvent(GameEvent.Ui ui) {
          return new AudioEventParameter((GameEvent.GenericEvent)ui, GameEvent.EventGroupType.Ui);
        }

        public static AudioEventParameter SFXEvent(GameEvent.Sfx sfx) {
          return new AudioEventParameter((GameEvent.GenericEvent)sfx, GameEvent.EventGroupType.Sfx);
        }

        // Unity doesn't like uints for some reason
        [SerializeField]
        private string _Event;

        [SerializeField]
        private string _EventType;

        private AudioEventParameter(GameEvent.GenericEvent evt, GameEvent.EventGroupType evtType) {
          _Event = evt.ToString();
          _EventType = evtType.ToString();
        }

        public GameEvent.GenericEvent Event {
          get {
            // if we used the default, return invalid
            if (string.IsNullOrEmpty(_Event)) {
              return GameEvent.GenericEvent.Invalid;
            }

            return (GameEvent.GenericEvent)Enum.Parse(typeof(GameEvent.GenericEvent), _Event);
          }
          set {
            _Event = value.ToString();
          }
        }

        public GameEvent.EventGroupType EventType {
          get {
            // if we used the default, return generic
            if (string.IsNullOrEmpty(_EventType)) {
              return GameEvent.EventGroupType.GenericEvent;
            }

            return (GameEvent.EventGroupType)Enum.Parse(typeof(GameEvent.EventGroupType), _EventType);
          }
          set {
            _EventType = value.ToString();
          }
        }

        public GameObjectType GetGameObjectType() {
          var eventType = EventType;

          switch (eventType) {
          case Anki.Cozmo.Audio.GameEvent.EventGroupType.Sfx:
            return GameObjectType.SFX;

          case Anki.Cozmo.Audio.GameEvent.EventGroupType.Ui:
            return GameObjectType.UI;

          // Only SFX and UI are valid for the time being
          case Anki.Cozmo.Audio.GameEvent.EventGroupType.Coz_App:
          case Anki.Cozmo.Audio.GameEvent.EventGroupType.Dev_Device:
          case Anki.Cozmo.Audio.GameEvent.EventGroupType.Dev_Robot:
          case Anki.Cozmo.Audio.GameEvent.EventGroupType.GenericEvent:
          case Anki.Cozmo.Audio.GameEvent.EventGroupType.Music:
          case Anki.Cozmo.Audio.GameEvent.EventGroupType.Robot_Sfx:
          case Anki.Cozmo.Audio.GameEvent.EventGroupType.Robot_Vo:
          default:
            return GameObjectType.Invalid;
          }
        }

        public bool IsInvalid() {
          return GameObjectType.Invalid == GetGameObjectType();
        }
      }

      public class GameAudioClient {

        // If you want to listen to all audio callback events register with Audio Client
        //    AudioClient client = AudioClient.Instance;
        //    client.OnAudioCallback += YourHandler

        static public ushort PostAudioEvent(AudioEventParameter parameter,
                                            Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                            CallbackHandler handler = null) {
          UnityAudioClient client = UnityAudioClient.Instance;
          return client.PostEvent(parameter.Event, parameter.GetGameObjectType(), callbackFlag, handler);
        }

        static public ushort PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui audioEvent,
                                         Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                         CallbackHandler handler = null) {
          UnityAudioClient client = UnityAudioClient.Instance;
          return client.PostEvent((GameEvent.GenericEvent)audioEvent, Anki.Cozmo.Audio.GameObjectType.UI, callbackFlag, handler);
        }

        static public ushort PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx audioEvent,
                                          Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                          CallbackHandler handler = null) {
          UnityAudioClient client = UnityAudioClient.Instance;
          return client.PostEvent((GameEvent.GenericEvent)audioEvent, Anki.Cozmo.Audio.GameObjectType.SFX, callbackFlag, handler);
        }

        static public ushort PostAnnouncerVOEvent(Anki.Cozmo.Audio.GameEvent.GenericEvent audioEvent,
                                                  Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                                  CallbackHandler handler = null) {
          UnityAudioClient client = UnityAudioClient.Instance;
          return client.PostEvent(audioEvent, Anki.Cozmo.Audio.GameObjectType.Aria, callbackFlag, handler);
        }

        // Remove callback handle from Audio Client
        static public void UnregisterCallbackHandler(ushort playId) {
          UnityAudioClient client = UnityAudioClient.Instance;
          client.UnregisterCallbackHandler(playId);
        }

        static public void SetVolumeValue(VolumeParameters.VolumeType parameter, float volume, int timeInMS = 0, CurveType curve = CurveType.Linear, bool storeValue = true) {
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
            client.PostParameter((GameParameter.ParameterType)parameter, volume, GameObjectType.Invalid, timeInMS, curve);
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
            SetVolumeValue(aParameter, aValue, 0, CurveType.Linear, false);
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
        static public void SetMusicState(Anki.Cozmo.Audio.GameState.Music state,
                                         bool interrupt = false,
                                         uint minDurationInMilliSeconds = 0) {
          UnityAudioClient client = UnityAudioClient.Instance;
          client.PostMusicState((GameState.GenericState)state, interrupt, minDurationInMilliSeconds);
        }

       
        // Game Helpers
        static public void SetMusicRoundState(int round) {
          SwitchState.Gameplay_Round roundState = SwitchState.Gameplay_Round.Invalid;
          switch (round) {
          case 1:
            roundState = SwitchState.Gameplay_Round.Round_01;
            break;
          case 2:
            roundState = SwitchState.Gameplay_Round.Round_02;
            break;
          case 3:
            roundState = SwitchState.Gameplay_Round.Round_03;
            break;
          case 4:
            roundState = SwitchState.Gameplay_Round.Round_04;
            break;
          case 5:
            roundState = SwitchState.Gameplay_Round.Round_05;
            break;
          default:
            // Set default state
            roundState = SwitchState.Gameplay_Round.Round_01;
            DAS.Error("GameAudioClient.SetMusicRoundState", string.Format("Unhandled round value: {0}", round));
            break;
          }

          UnityAudioClient client = UnityAudioClient.Instance;
          client.PostSwitchState (SwitchState.SwitchGroupType.Gameplay_Round, (SwitchState.GenericSwitch)roundState, GameObjectType.Default);
        }

        static public void SetSparkedMusicState(Anki.Cozmo.Audio.SwitchState.Sparked sparked) {
          UnityAudioClient client = UnityAudioClient.Instance;
          client.PostSwitchState (SwitchState.SwitchGroupType.Sparked, (SwitchState.GenericSwitch)sparked, GameObjectType.Default);
        }
     
      }
    }
  }
}

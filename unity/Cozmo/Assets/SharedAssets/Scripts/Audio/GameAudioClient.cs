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

        public static AudioEventParameter DefaultClick = new AudioEventParameter(
                                                           GameEvent.GenericEvent.Play__Ui__Click_General,
                                                           GameEvent.EventGroupType.Ui,
                                                           GameObjectType.UI);

        public static AudioEventParameter UIEvent(GameEvent.Ui ui) {
          return new AudioEventParameter(
            (GameEvent.GenericEvent)ui,
            GameEvent.EventGroupType.Ui,
            GameObjectType.UI);
        }

        public static AudioEventParameter VOEvent(GameEvent.GenericEvent evt) {
          return new AudioEventParameter(
            evt,
            GameEvent.EventGroupType.GenericEvent,
            GameObjectType.Aria);
        }

        public static AudioEventParameter SFXEvent(GameEvent.Sfx sfx) {
          return new AudioEventParameter(
            (GameEvent.GenericEvent)sfx,
            GameEvent.EventGroupType.Sfx,
            GameObjectType.SFX);
        }

        // Unity doesn't like uints for some reason
        [SerializeField]
        private string _Event;

        [SerializeField]
        private string _EventType;

        [SerializeField]
        private int _GameObjectType;

        private AudioEventParameter(GameEvent.GenericEvent evt, GameEvent.EventGroupType evtType, GameObjectType gameObjectType) {
          _Event = evt.ToString();
          _EventType = evtType.ToString();
          _GameObjectType = (int)(uint)gameObjectType;
        }

        public GameEvent.GenericEvent Event {
          get {
            return (GameEvent.GenericEvent)Enum.Parse(typeof(GameEvent.GenericEvent), _Event);
          }
          set {
            _Event = value.ToString();
          }
        }

        public GameEvent.EventGroupType EventType {
          get {
            return (GameEvent.EventGroupType)Enum.Parse(typeof(GameEvent.EventGroupType), _EventType);
          }
          set {
            _EventType = value.ToString();
          }
        }

        public GameObjectType GameObjectType {
          get {
            return (GameObjectType)(uint)_GameObjectType;
          }
          set {
            _GameObjectType = (int)(uint)value;
          }
        }

        public bool IsInvalid() {
          return GameObjectType == GameObjectType.Invalid;
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
          return client.PostEvent(parameter.Event, parameter.GameObjectType, callbackFlag, handler);
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
            System.Collections.Generic.Dictionary<VolumeParameters.VolumeType, float> volumePrefs = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.VolumePreferences;
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
          System.Collections.Generic.Dictionary<VolumeParameters.VolumeType, float> volumePrefs = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.VolumePreferences;
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

        // Define default volume values
        static private float GetDefaultVolume(VolumeParameters.VolumeType volType) {
          float value = 1.0f;
          switch (volType) {
          case Anki.Cozmo.Audio.VolumeParameters.VolumeType.Music:
            value = 1.0f;
            break;

          case Anki.Cozmo.Audio.VolumeParameters.VolumeType.Robot: {
#if UNITY_EDITOR
              value = 0.6f;
#else
              value = 1.0f;
#endif
            }
            break;

          case Anki.Cozmo.Audio.VolumeParameters.VolumeType.SFX:
            value = 1.0f;
            break;

          case Anki.Cozmo.Audio.VolumeParameters.VolumeType.UI:
            value = 1.0f;
            break;

          case Anki.Cozmo.Audio.VolumeParameters.VolumeType.VO:
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
      }
    }
  }
}

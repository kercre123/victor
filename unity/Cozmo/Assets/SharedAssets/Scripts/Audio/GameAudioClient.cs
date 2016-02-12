using UnityEngine;
using System.Collections;

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
          GameEvent.GenericEvent.Sfx_Ui_Click_General_Play,
          GameEvent.EventGroupType.UI,
          GameObjectType.UI);

        public static AudioEventParameter UIEvent(GameEvent.UI ui) {
          return new AudioEventParameter(
            (GameEvent.GenericEvent)ui,
            GameEvent.EventGroupType.UI,
            GameObjectType.UI);
        }

        public static AudioEventParameter VOEvent(GameEvent.GenericEvent evt) {
          return new AudioEventParameter(
            evt,
            GameEvent.EventGroupType.GenericEvent,
            GameObjectType.Aria);
        }

        public static AudioEventParameter SFXEvent(GameEvent.SFX sfx) {
          return new AudioEventParameter(
            (GameEvent.GenericEvent)sfx,
            GameEvent.EventGroupType.SFX,
            GameObjectType.SFX);
        }

        // Unity doesn't like uints for some reason
        [SerializeField]
        private int _Event;

        [SerializeField]
        private int _EventType;

        [SerializeField]
        private int _GameObjectType;

        private AudioEventParameter(GameEvent.GenericEvent evt, GameEvent.EventGroupType evtType, GameObjectType gameObjectType) {
          _Event = (int)(uint)evt;
          _EventType = (int)(uint)evtType;
          _GameObjectType = (int)(uint)gameObjectType;
        }

        public GameEvent.GenericEvent Event {
          get {
            return (GameEvent.GenericEvent)(uint)_Event;
          }
          set {
            _Event = (int)(uint)value;
          }
        }

        public GameEvent.EventGroupType EventType {
          get {
            return (GameEvent.EventGroupType)(uint)_EventType;
          }
          set {
            _EventType = (int)(uint)value;
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
          AudioClient client = AudioClient.Instance;
          return client.PostEvent(parameter.Event, parameter.GameObjectType, callbackFlag, handler);
        }

        static public ushort PostUIEvent(Anki.Cozmo.Audio.GameEvent.UI audioEvent,
                                         Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                         CallbackHandler handler = null) {
          AudioClient client = AudioClient.Instance;
          return client.PostEvent((GameEvent.GenericEvent)audioEvent, Anki.Cozmo.Audio.GameObjectType.UI, callbackFlag, handler);
        }

        static public ushort PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX audioEvent,
                                          Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                          CallbackHandler handler = null) {
          AudioClient client = AudioClient.Instance;
          return client.PostEvent((GameEvent.GenericEvent)audioEvent, Anki.Cozmo.Audio.GameObjectType.SFX, callbackFlag, handler);
        }

        static public ushort PostAnnouncerVOEvent(Anki.Cozmo.Audio.GameEvent.GenericEvent audioEvent,
                                                  Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                                  CallbackHandler handler = null) {
          AudioClient client = AudioClient.Instance;
          return client.PostEvent(audioEvent, Anki.Cozmo.Audio.GameObjectType.Aria, callbackFlag, handler);
        }

        // Don't think we are going to allow playing cozmo vo from ui, currently they must be played form an animation
        /*
        static public ushort PostCozmoVOEvent(Anki.Cozmo.Audio.GameEvent.GenericEvent audioEvent,
                                              Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                              CallbackHandler handler = null) {
          AudioClient client = AudioClient.Instance;
          return client.PostEvent(audioEvent, Anki.Cozmo.Audio.GameObjectType.Default, callbackFlag, handler);
        }
        */

        // Remove callback handle from Audio Client
        static public void UnregisterCallbackHandler(ushort playId) {
          AudioClient client = AudioClient.Instance;
          client.UnregisterCallbackHandler(playId);
        }

        static public void SetVolumeValue(VolumeParameters.VolumeType parameter, float volume, int timeInMS = 0, CurveType curve = CurveType.Linear ) {
          // TODO Fix GameObject Id -- JMR Do I still need this?
          AudioClient client = AudioClient.Instance;
          // TODO Need to cast for Invalid GameObj to set global RTPCs -- JMR Do I still need this?
          client.PostParameter((GameParameter.ParameterType)parameter, volume, GameObjectType.Invalid, timeInMS, curve);
        }

        static public void SetMusicVolume(float volume, int timeInMS = 0, CurveType curve = CurveType.Linear ) {
          AudioClient client = AudioClient.Instance;
          client.PostParameter(GameParameter.ParameterType.Music_Volume, volume, GameObjectType.Invalid, timeInMS, curve);
        }

        // Set Music States
        // We can move this, I just need a place to keep static state to start the music
        private static bool _didPlayMusic = false; 
        static public void SetMusicState(Anki.Cozmo.Audio.GameState.Music state) {
          AudioClient client = AudioClient.Instance;
          if (!_didPlayMusic) {
            client.PostEvent((GameEvent.GenericEvent)GameEvent.Music.Play, GameObjectType.Default);
            _didPlayMusic = true;
          }
          client.PostGameState(GameState.StateGroupType.Music, (GameState.GenericState)state);
        }
      }
    }
  }
}

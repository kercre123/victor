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

      public class GameAudioClient {

        // If you want to listen to all audio callback events register with Audio Client
        //    AudioClient client = AudioClient.Instance;
        //    client.OnAudioCallback += YourHandler

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

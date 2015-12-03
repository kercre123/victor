using UnityEngine;
using System.Collections;

namespace Anki {
  namespace Cozmo {
    namespace Audio {

      public class GameAudioClient {

        // If you want to listen to all audio callback events register with Audio Client
        //    AudioClient client = AudioClient.Instance;
        //    client.OnAudioCallback += YourHandler

        static public ushort PostUIEvent(Anki.Cozmo.Audio.EventType audioEvent,
                                         Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                         CallbackHandler handler = null) {
          // TODO Fix GameObject Id
          AudioClient client = AudioClient.Instance;
          return client.PostEvent(audioEvent, 0, callbackFlag, handler);
        }

        static public ushort PostSFXEvent(Anki.Cozmo.Audio.EventType audioEvent,
                                          Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                          CallbackHandler handler = null) {
          // TODO Fix GameObject Id
          AudioClient client = AudioClient.Instance;
          return client.PostEvent(audioEvent, 0, callbackFlag, handler);
        }

        static public ushort PostAnnouncerVOEvent(Anki.Cozmo.Audio.EventType audioEvent,
                                                  Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                                  CallbackHandler handler = null) {
          // TODO Fix GameObject Id
          AudioClient client = AudioClient.Instance;
          return client.PostEvent(audioEvent, 0, callbackFlag, handler);
        }

        static public ushort PostCozmoVOEvent(Anki.Cozmo.Audio.EventType audioEvent,
                                              Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                              CallbackHandler handler = null) {
          // TODO Fix GameObject Id
          AudioClient client = AudioClient.Instance;
          return client.PostEvent(audioEvent, 0, callbackFlag, handler);
        }

        // Remove callback handle from Audio Client
        static public void UnregisterCallbackHandler(ushort playId) {
          AudioClient client = AudioClient.Instance;
          client.UnregisterCallbackHandler(playId);
        }

        static public void SetVolumeValue(ParameterType parameter, float volume ) {
          // TODO Fix GameObject Id
          AudioClient client = AudioClient.Instance;
          client.PostParameter(parameter, volume, 0); // TODO Need to const for Invalid GameObj to set global RTPCs
        }

      }
    }
  }
}

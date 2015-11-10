using UnityEngine;
using System.Collections;
using Anki.Cozmo.Audio;

namespace Anki {
  namespace Cozmo {
    namespace Audio {
      
      public class AudioClient {

        private static AudioClient _audioClient = null;
        private RobotEngineManager _robotEngineManager = null;
        private bool _isInitialized = false;

        public static AudioClient Instance {
          get {
            if (_audioClient == null) {
              _audioClient = new AudioClient();
              _audioClient.Initialize();
            }
            return _audioClient;
          }
        }

        public void Initialize() {
          if (_isInitialized) {
            return;
          }
          // Setup Audio Controller
          _robotEngineManager = RobotEngineManager.instance;
          // Setup Engine To Game callbacks
          _robotEngineManager.ReceivedAudioCallbackDuration += HandleCallback;
          _robotEngineManager.ReceivedAudioCallbackMarker += HandleCallback;
          _robotEngineManager.ReceivedAudioCallbackComplete += HandleCallback;
          _isInitialized = true;
        }

        ~AudioClient() {
          _robotEngineManager.ReceivedAudioCallbackDuration -= HandleCallback;
          _robotEngineManager.ReceivedAudioCallbackMarker -= HandleCallback;
          _robotEngineManager.ReceivedAudioCallbackComplete -= HandleCallback;

          _robotEngineManager = null;
          _isInitialized = false;
        }

        public void PostEvent(Anki.Cozmo.Audio.EventType audioEvent,
                        ushort gameObjectId,
                        Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone) {
          DAS.Info("AudioController.PostAudioEvent", "Event: " + audioEvent.ToString() + "  gameObjId: " +
          gameObjectId + " CallbackFlag: " + callbackFlag.GetHashCode());

          PostAudioEvent msg = new PostAudioEvent(audioEvent, gameObjectId, callbackFlag);
          _robotEngineManager.Message.PostAudioEvent = msg;
          _robotEngineManager.SendMessage();
        }

        public void PostGameState(Anki.Cozmo.Audio.GameStateType gameState) {
          DAS.Info("AudioController.PostAudioGameState", "GameState: " + gameState.ToString());
          PostAudioGameState msg = new PostAudioGameState(gameState);
          _robotEngineManager.Message.PostAudioGameState = msg;
          _robotEngineManager.SendMessage();
        }

        public void PostSwitchState(Anki.Cozmo.Audio.SwitchStateType switchState, ushort gameObjectId) {
          DAS.Info("AudioController.PostAudioSwitchState", "SwitchState: " + switchState.ToString() +
          " gameObjId: " + gameObjectId);
          PostAudioSwitchState msg = new PostAudioSwitchState(switchState, gameObjectId);
          _robotEngineManager.Message.PostAudioSwitchState = msg;
          _robotEngineManager.SendMessage();
        }

        public void PostParameter(Anki.Cozmo.Audio.ParameterType parameter,
                            float parameterValue,
                            ushort gameObjectId,
                            int timeInMilliSeconds = 0,
                            Anki.Cozmo.Audio.CurveType curve = CurveType.Linear) {
          DAS.Info("AudioController.PostAudioParameter", "Parameter: " + parameter.ToString() + " value: " + parameterValue +
          " gameObjId: " + gameObjectId + " timeInMilliSec: " + timeInMilliSeconds + " curve: " + curve);
          PostAudioParameter msg = new PostAudioParameter(parameter, parameterValue, gameObjectId, timeInMilliSeconds, curve);
          _robotEngineManager.Message.PostAudioParameter = msg;
          _robotEngineManager.SendMessage();
        }

        private void HandleCallback(AudioCallbackDuration message) {
          DAS.Info("AudioController.HandleCallback", "Received Audio Callback Duration " + message.ToString());
          // TODO: Do stuff
        }

        private void HandleCallback(AudioCallbackMarker message) {
          DAS.Info("AudioController.HandleCallback", "Received Audio Callback Marker " + message.ToString());
          // TODO: Do stuff
        }

        private void HandleCallback(AudioCallbackComplete message) {
          DAS.Info("AudioController.HandleCallback", "Received Audio Callback Complete " + message.ToString());
          // TODO: Do stuff
        }
      }
    }
  }
}

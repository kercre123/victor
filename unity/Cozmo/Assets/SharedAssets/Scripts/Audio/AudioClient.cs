using UnityEngine;
using System.Collections;
using Anki.Cozmo.Audio;

namespace Anki {
  namespace Cozmo {
    namespace Audio {
      
      public class AudioClient {

        private static AudioClient _AudioClient = null;
        private RobotEngineManager _RobotEngineManager = null;
        private bool _IsInitialized = false;

        public static AudioClient Instance {
          get {
            if (_AudioClient == null) {
              _AudioClient = new AudioClient();
              _AudioClient.Initialize();
            }
            return _AudioClient;
          }
        }

        public void Initialize() {
          if (_IsInitialized) {
            return;
          }
          // Setup Audio Controller
          _RobotEngineManager = RobotEngineManager.instance;
          // Setup Engine To Game callbacks
          _RobotEngineManager.ReceivedAudioCallbackDuration += HandleCallback;
          _RobotEngineManager.ReceivedAudioCallbackMarker += HandleCallback;
          _RobotEngineManager.ReceivedAudioCallbackComplete += HandleCallback;
          _IsInitialized = true;
        }

        ~AudioClient() {
          _RobotEngineManager.ReceivedAudioCallbackDuration -= HandleCallback;
          _RobotEngineManager.ReceivedAudioCallbackMarker -= HandleCallback;
          _RobotEngineManager.ReceivedAudioCallbackComplete -= HandleCallback;

          _RobotEngineManager = null;
          _IsInitialized = false;
        }

        public void PostEvent(Anki.Cozmo.Audio.EventType audioEvent,
                              ushort gameObjectId,
                              Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone) {
          DAS.Info("AudioController.PostAudioEvent", "Event: " + audioEvent.ToString() + "  gameObjId: " +
          gameObjectId + " CallbackFlag: " + callbackFlag.GetHashCode());

          PostAudioEvent msg = new PostAudioEvent(audioEvent, gameObjectId, callbackFlag);
          _RobotEngineManager.Message.PostAudioEvent = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostGameState(Anki.Cozmo.Audio.GameStateType gameState) {
          DAS.Info("AudioController.PostAudioGameState", "GameState: " + gameState.ToString());
          PostAudioGameState msg = new PostAudioGameState(gameState);
          _RobotEngineManager.Message.PostAudioGameState = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostSwitchState(Anki.Cozmo.Audio.SwitchStateType switchState, ushort gameObjectId) {
          DAS.Info("AudioController.PostAudioSwitchState", "SwitchState: " + switchState.ToString() +
          " gameObjId: " + gameObjectId);
          PostAudioSwitchState msg = new PostAudioSwitchState(switchState, gameObjectId);
          _RobotEngineManager.Message.PostAudioSwitchState = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostParameter(Anki.Cozmo.Audio.ParameterType parameter,
                                  float parameterValue,
                                  ushort gameObjectId,
                                  int timeInMilliSeconds = 0,
                                  Anki.Cozmo.Audio.CurveType curve = CurveType.Linear) {
          DAS.Info("AudioController.PostAudioParameter", "Parameter: " + parameter.ToString() + " value: " + parameterValue +
          " gameObjId: " + gameObjectId + " timeInMilliSec: " + timeInMilliSeconds + " curve: " + curve);
          PostAudioParameter msg = new PostAudioParameter(parameter, parameterValue, gameObjectId, timeInMilliSeconds, curve);
          _RobotEngineManager.Message.PostAudioParameter = msg;
          _RobotEngineManager.SendMessage();
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

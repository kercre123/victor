using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using sysDebug = System.Diagnostics.Debug;

namespace Anki {
  namespace Cozmo {
    namespace Audio {

      public delegate void CallbackHandler(CallbackInfo callbackInfo);

      public class AudioClient {
        
        public event CallbackHandler OnAudioCallback;

        private static AudioClient _AudioClient = null;
        private RobotEngineManager _RobotEngineManager = null;
        private bool _IsInitialized = false;

        public static AudioClient Instance {
          get {
            if (_AudioClient == null) {
              _AudioClient = new AudioClient();
            }
            _AudioClient.Initialize();
            return _AudioClient;
          }
        }

        public void Initialize() {
          if (_IsInitialized) {
            return;
          }
          // Setup Audio Controller
          _RobotEngineManager = RobotEngineManager.Instance;
          // Setup Engine To Game callbacks
          if (null != _RobotEngineManager) {
            _RobotEngineManager.ReceivedAudioCallbackDuration += HandleCallback;
            _RobotEngineManager.ReceivedAudioCallbackMarker += HandleCallback;
            _RobotEngineManager.ReceivedAudioCallbackComplete += HandleCallback;
            _IsInitialized = true;
          }
          else {
            DAS.Warn("AudioClient.Initialize", "Failed to Initialize!");
          }
        }

        ~AudioClient() {
          _RobotEngineManager.ReceivedAudioCallbackDuration -= HandleCallback;
          _RobotEngineManager.ReceivedAudioCallbackMarker -= HandleCallback;
          _RobotEngineManager.ReceivedAudioCallbackComplete -= HandleCallback;

          _RobotEngineManager = null;
          _IsInitialized = false;
        }

        // Basic Audio Client Operations
        // Return PlayId - Note: PlayId is not guaranteed to be unique it will eventually roll over.
        public ushort PostEvent(Anki.Cozmo.Audio.EventType audioEvent,
                                ushort gameObjectId,
                                Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                CallbackHandler handler = null) {
          DAS.Debug("AudioController.PostAudioEvent", "Event: " + audioEvent.ToString() + "  gameObjId: " +
                   gameObjectId + " CallbackFlag: " + callbackFlag);
          ushort playId = _GetPlayId();
          PostAudioEvent msg = new PostAudioEvent(audioEvent, gameObjectId, callbackFlag, playId);
          _RobotEngineManager.Message.PostAudioEvent = msg;
          _RobotEngineManager.SendMessage();

          // Assert if a callback handle is passed in and callback flag is set to EventNone
          sysDebug.Assert( !(AudioCallbackFlag.EventNone == callbackFlag && null != handler) );
          if (null != handler) {
            AddCallbackHandler(playId, callbackFlag, handler);
          }

          return playId;
        }

        public void PostGameState(Anki.Cozmo.Audio.GameStateGroupType gameStateGroup,
                                  Anki.Cozmo.Audio.GameStateType gameState) {
          DAS.Debug("AudioController.PostAudioGameState", "GameState: " + gameState.ToString());
          PostAudioGameState msg = new PostAudioGameState(gameStateGroup, gameState);
          _RobotEngineManager.Message.PostAudioGameState = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostSwitchState(Anki.Cozmo.Audio.SwitchStateGroupType switchStateGroup, Anki.Cozmo.Audio.SwitchStateType switchState, ushort gameObjectId) {
          DAS.Debug("AudioController.PostAudioSwitchState", "SwitchState: " + switchState.ToString() +
          " gameObjId: " + gameObjectId);
          PostAudioSwitchState msg = new PostAudioSwitchState(switchStateGroup, switchState, gameObjectId);
          _RobotEngineManager.Message.PostAudioSwitchState = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostParameter(Anki.Cozmo.Audio.ParameterType parameter,
                                  float parameterValue,
                                  ushort gameObjectId,
                                  int timeInMilliSeconds = 0,
                                  Anki.Cozmo.Audio.CurveType curve = CurveType.Linear) {
          DAS.Debug("AudioController.PostAudioParameter", "Parameter: " + parameter.ToString() + " value: " + parameterValue +
          " gameObjId: " + gameObjectId + " timeInMilliSec: " + timeInMilliSeconds + " curve: " + curve);
          PostAudioParameter msg = new PostAudioParameter(parameter, parameterValue, gameObjectId, timeInMilliSeconds, curve);
          _RobotEngineManager.Message.PostAudioParameter = msg;
          _RobotEngineManager.SendMessage();
        }

        // Callback functionality
        private const ushort kInvalidPlayId = 0;
        private ushort _previousPlayId = kInvalidPlayId;

        private struct CallbackBundle {
          public AudioCallbackFlag Flags;
          public CallbackHandler Handler;

          public CallbackBundle(AudioCallbackFlag flags, CallbackHandler handler) {
            this.Flags = flags;
            this.Handler = handler;
          }
        }
        private Dictionary<ushort, CallbackBundle> _callbackDelegates = new Dictionary<ushort, CallbackBundle>();

        // Helpers
        private ushort _GetPlayId() {
          ++_previousPlayId;
          // Allow callback ids to wrap
          if (kInvalidPlayId == _previousPlayId) {
            ++_previousPlayId;
          }
          return _previousPlayId;
        }

        private void AddCallbackHandler(ushort playId, AudioCallbackFlag flags, CallbackHandler handler) {
          _callbackDelegates.Add(playId, new CallbackBundle(flags, handler));
          DAS.Debug("AudioClient.AddCallbackHandler", "Add Callback Bundle with PlayId " + playId.ToString() + " Flags " + flags.ToString());
        }

        public void UnregisterCallbackHandler(ushort playId) {
          bool success = _callbackDelegates.Remove(playId);
          if (success) {
            DAS.Debug("AudioClient.UnregisterCallbackHandler", "Removed Callback Bundle with PlayId " + playId.ToString());
          }
          else {
            DAS.Warn("AudioClient.UnregisterCallbackHandler", "Failed to Remove Callback Bundle with PlayId " + playId.ToString());
          }
        }

        // Will automatically unregister callback handle on last registered event if unregisterHandle == null 
        private void PerformCallbackHandler(ushort playId, CallbackInfo info, bool? unregisterHandle = null) {
          CallbackBundle callbackBundle;
          if (_callbackDelegates.TryGetValue(playId, out callbackBundle)) {
            callbackBundle.Handler(info);
            // Auto unregister Event Complete or if only if last event
            if (null == unregisterHandle) {
              if ( (AudioCallbackFlag.EventComplete & info.CallbackType) == AudioCallbackFlag.EventComplete ||
                  AudioCallbackFlag.EventDuration == callbackBundle.Flags) {
                UnregisterCallbackHandler(playId);
              }
            }
            else if ( unregisterHandle.Value ) {
              UnregisterCallbackHandler(playId);
            }
          }
          else {
            DAS.Warn("AudioClient.PerformCallbackHandler", "Could not find Callback Handler for PlayId: " + playId.ToString());
          }
        }
      
        // Handle message types
        private void HandleCallback(AudioCallbackDuration message) {
          DAS.Debug("AudioController.HandleCallback", "Received Audio Callback Duration " + message.ToString());
          CallbackInfo info = new CallbackInfo(message);
          if (null != OnAudioCallback) {
            OnAudioCallback(info);
          }
          // Call back handle
          PerformCallbackHandler(info.PlayId, info);
        }

        private void HandleCallback(AudioCallbackMarker message) {
          DAS.Debug("AudioController.HandleCallback", "Received Audio Callback Marker " + message.ToString());
          CallbackInfo info = new CallbackInfo(message);
          if (null != OnAudioCallback) {
            OnAudioCallback(info);
          }
          // Call back handle
          PerformCallbackHandler(info.PlayId, info);
        }

        private void HandleCallback(AudioCallbackComplete message) {
          DAS.Debug("AudioController.HandleCallback", "Received Audio Callback Complete " + message.ToString());
          CallbackInfo info = new CallbackInfo(message);
          if (null != OnAudioCallback) {
            OnAudioCallback(info);
          }
          // Call back handle
          PerformCallbackHandler(info.PlayId, info);
        }

        private void HandleCallback(AudioCallbackError message) {
          DAS.Debug("AudioController.HandleCallback", "Received Audio Callback Error " + message.ToString());
          CallbackInfo info = new CallbackInfo(message);
          if (null != OnAudioCallback) {
            OnAudioCallback(info);
          }
          // Call back handle
          PerformCallbackHandler(info.PlayId, info);
        }

        // Data Helpers
        private List<Anki.Cozmo.Audio.EventType> _Events;
        private List<Anki.Cozmo.Audio.GameStateGroupType> _GameStateGroups;
        private Dictionary<Anki.Cozmo.Audio.GameStateGroupType, List<Anki.Cozmo.Audio.GameStateType>> _GameStateTypes;
        private List<Anki.Cozmo.Audio.SwitchStateGroupType> _SwitchStateGroups;
        private Dictionary<Anki.Cozmo.Audio.SwitchStateGroupType, List<Anki.Cozmo.Audio.SwitchStateType>> _SwitchStateTypes;
        private List<Anki.Cozmo.Audio.ParameterType> _RTPCParameters;


        public List<Anki.Cozmo.Audio.EventType> GetEvents() {
          if (null == _Events) {
            _Events = Enum.GetValues(typeof(Anki.Cozmo.Audio.EventType)).Cast<Anki.Cozmo.Audio.EventType>().ToList();
            _Events.Sort(delegate (EventType a, EventType b) {
              if (EventType.Invalid == a) return -1;
              else if (EventType.Invalid == b) return 1;
              else return a.ToString().CompareTo(b.ToString());
            });
          }

          return _Events;
        }

        public List<Anki.Cozmo.Audio.GameStateGroupType> GetGameStateGroups() {
          if (null == _GameStateGroups) {
            _GameStateGroups = Enum.GetValues(typeof(Anki.Cozmo.Audio.GameStateGroupType)).Cast<Anki.Cozmo.Audio.GameStateGroupType>().ToList();
            _GameStateGroups.Sort(delegate (GameStateGroupType a, GameStateGroupType b) {
              if (GameStateGroupType.Invalid == a) return -1;
              else if (GameStateGroupType.Invalid == b) return 1;
              else return a.ToString().CompareTo(b.ToString());
            });
          }

          return _GameStateGroups;
        }

        public List<Anki.Cozmo.Audio.GameStateType> GetGameStates(Anki.Cozmo.Audio.GameStateGroupType stateGroup) {
          if (null == _GameStateTypes) {
            _GameStateTypes = new Dictionary<GameStateGroupType, List<GameStateType>>();
          }

          List<Anki.Cozmo.Audio.GameStateType> groupStates;
          if (_GameStateTypes.TryGetValue(stateGroup, out groupStates)) {
            return groupStates;
          }
            
          return null;
        }

        public List<Anki.Cozmo.Audio.SwitchStateGroupType> GetSwitchStateGroups() {
          if (null == _SwitchStateGroups) {
            _SwitchStateGroups = Enum.GetValues(typeof(Anki.Cozmo.Audio.SwitchStateGroupType)).Cast<Anki.Cozmo.Audio.SwitchStateGroupType>().ToList();
            _SwitchStateGroups.Sort(delegate (SwitchStateGroupType a, SwitchStateGroupType b) {
              if (SwitchStateGroupType.Invalid == a) return -1;
              else if (SwitchStateGroupType.Invalid == b) return 1;
              else return a.ToString().CompareTo(b.ToString());
            });
          }

          return _SwitchStateGroups;
        }

        public List<Anki.Cozmo.Audio.SwitchStateType> GetSwitchStates(Anki.Cozmo.Audio.SwitchStateGroupType stateGroup) {
          if (null == _SwitchStateTypes) {
            _SwitchStateTypes = new Dictionary<SwitchStateGroupType, List<SwitchStateType>>();
          }

          List<Anki.Cozmo.Audio.SwitchStateType> groupStates;
          if (_SwitchStateTypes.TryGetValue(stateGroup, out groupStates)) {
            return groupStates;
          }

          return null;
        }

        public List<Anki.Cozmo.Audio.ParameterType> GetParameters() {
          if (null == _RTPCParameters) {
            _RTPCParameters = Enum.GetValues(typeof(Anki.Cozmo.Audio.ParameterType)).Cast<Anki.Cozmo.Audio.ParameterType>().ToList();
            _RTPCParameters.Sort(delegate (ParameterType a, ParameterType b) {
              if (ParameterType.Invalid == a) return -1;
              else if (ParameterType.Invalid == b) return 1;
              else return a.ToString().CompareTo(b.ToString());
            });
          }

          return _RTPCParameters;
        }

      }
    }
  }
}

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
            _RobotEngineManager.ReceivedAudioCallback += HandleCallback;
            _IsInitialized = true;
          }
          else {
            DAS.Warn("AudioClient.Initialize", "Failed to Initialize!");
          }
        }

        ~AudioClient() {
          _RobotEngineManager.ReceivedAudioCallback -= HandleCallback;
          _RobotEngineManager = null;
          _IsInitialized = false;
        }

        // Basic Audio Client Operations
        // Return PlayId - Note: PlayId is not guaranteed to be unique it will eventually roll over.
        public ushort PostEvent(Anki.Cozmo.Audio.GenericEvent audioEvent,
                                Anki.Cozmo.Audio.GameObjectType gameObject,
                                Anki.Cozmo.Audio.AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                CallbackHandler handler = null) {
          DAS.Debug("AudioController.PostAudioEvent", "Event: " + audioEvent.ToString() + "  GameObj: " +
                    gameObject.ToString() + " CallbackFlag: " + callbackFlag);
          ushort playId = _GetPlayId();

          // Only register for callbacks if a flag is set.
          // Callbacks are only registered if callbackId != kInvalidPlayId
          ushort callbackId = AudioCallbackFlag.EventNone == callbackFlag ? kInvalidPlayId : playId;
          PostAudioEvent msg = new PostAudioEvent(audioEvent, gameObject, callbackId);
          _RobotEngineManager.Message.PostAudioEvent = msg;
          _RobotEngineManager.SendMessage();

          // Assert if a callback handle is passed in and callback flag is set to EventNone
          sysDebug.Assert( !(AudioCallbackFlag.EventNone == callbackFlag && null != handler) );
          if (null != handler) {
            AddCallbackHandler(playId, callbackFlag, handler);
          }

          return playId;
        }

        // Pass in game object type to stop audio events on that game object, use Invalid to stop all audio
        public void StopAllAudioEvents(Anki.Cozmo.Audio.GameObjectType gameObject = Anki.Cozmo.Audio.GameObjectType.Invalid) {
          DAS.Debug("AudioController.StopAllAudioEvents", "GameObj: " + gameObject.ToString());
          StopAllAudioEvents msg = new Anki.Cozmo.Audio.StopAllAudioEvents(gameObject);
          _RobotEngineManager.Message.StopAllAudioEvents = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostGameState(Anki.Cozmo.Audio.StateGroupType gameStateGroup,
                                  Anki.Cozmo.Audio.GenericState gameState) {
          DAS.Debug("AudioController.PostAudioGameState", "GameState: " + gameState.ToString());
          PostAudioGameState msg = new PostAudioGameState(gameStateGroup, gameState);
          _RobotEngineManager.Message.PostAudioGameState = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostSwitchState(Anki.Cozmo.Audio.SwitchGroupType switchStateGroup, Anki.Cozmo.Audio.GenericSwitch switchState, GameObjectType gameObject) {
          DAS.Debug("AudioController.PostAudioSwitchState", "SwitchState: " + switchState.ToString() +
                    " gameObj: " + gameObject.ToString());
          PostAudioSwitchState msg = new PostAudioSwitchState(switchStateGroup, switchState, gameObject);
          _RobotEngineManager.Message.PostAudioSwitchState = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostParameter(Anki.Cozmo.Audio.ParameterType parameter,
                                  float parameterValue,
                                  GameObjectType gameObject,
                                  int timeInMilliSeconds = 0,
                                  Anki.Cozmo.Audio.CurveType curve = CurveType.Linear) {
          DAS.Debug("AudioController.PostAudioParameter", "Parameter: " + parameter.ToString() + " Value: " + parameterValue +
                    " GameObj: " + gameObject.ToString() + " TimeInMilliSec: " + timeInMilliSeconds + " Curve: " + curve);
          PostAudioParameter msg = new PostAudioParameter(parameter, parameterValue, gameObject, timeInMilliSeconds, curve);
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
            // Only perform callback that the caller requested or an error
            AudioCallbackFlag callbackType = info.CallbackType;
            if (((callbackBundle.Flags & callbackType) == callbackType) || AudioCallbackFlag.EventError == callbackType) {
              callbackBundle.Handler(info);
            }
            // Auto unregister Event
            // Unregister if callback handle if this is the completion or error callback
            // FIXME: Waiting to hear back form WWise if Completeion callback is allways called after and error callback
            if (null == unregisterHandle) {
              if ((AudioCallbackFlag.EventComplete & callbackType) == AudioCallbackFlag.EventComplete ||
                  (AudioCallbackFlag.EventError & callbackType) == AudioCallbackFlag.EventError) {
                UnregisterCallbackHandler(playId);
              }
            }
            else if ( unregisterHandle.Value ) {
              UnregisterCallbackHandler(playId);
            }
          }
        }
      
        // Handle message types
        private void HandleCallback(AudioCallback message) {
          DAS.Debug("AudioController.HandleCallback", "Received Audio Callback " + message.ToString());
          CallbackInfo info = new CallbackInfo(message);
          if (null != OnAudioCallback) {
            OnAudioCallback(info);
          }
          // Call back handle
          PerformCallbackHandler(info.PlayId, info);
        }

        // Data Helpers
        private List<Anki.Cozmo.Audio.GameObjectType> _GameObjects;
        private List<Anki.Cozmo.Audio.GenericEvent> _Events;
        private List<Anki.Cozmo.Audio.StateGroupType> _GameStateGroups;
        private Dictionary<Anki.Cozmo.Audio.StateGroupType, List<Anki.Cozmo.Audio.GenericState>> _GameStateTypes;
        private List<Anki.Cozmo.Audio.SwitchGroupType> _SwitchStateGroups;
        private Dictionary<Anki.Cozmo.Audio.SwitchGroupType, List<Anki.Cozmo.Audio.GenericSwitch>> _SwitchStateTypes;
        private List<Anki.Cozmo.Audio.ParameterType> _RTPCParameters;


        public List<Anki.Cozmo.Audio.GameObjectType> GetGameObjects() {
          if (null == _GameObjects) {
            _GameObjects = Enum.GetValues(typeof(Anki.Cozmo.Audio.GameObjectType)).Cast<Anki.Cozmo.Audio.GameObjectType>().ToList();
          }
          return _GameObjects;
        }

        public List<Anki.Cozmo.Audio.GenericEvent> GetEvents() {
          if (null == _Events) {
            _Events = Enum.GetValues(typeof(Anki.Cozmo.Audio.GenericEvent)).Cast<Anki.Cozmo.Audio.GenericEvent>().ToList();
            _Events.Sort(delegate (GenericEvent a, GenericEvent b) {
              if (GenericEvent.Invalid == a) return -1;
              else if (GenericEvent.Invalid == b) return 1;
              else return a.ToString().CompareTo(b.ToString());
            });
          }

          return _Events;
        }

        public List<Anki.Cozmo.Audio.StateGroupType> GetGameStateGroups() {
          if (null == _GameStateGroups) {
            _GameStateGroups = Enum.GetValues(typeof(Anki.Cozmo.Audio.StateGroupType)).Cast<Anki.Cozmo.Audio.StateGroupType>().ToList();
            _GameStateGroups.Sort(delegate (StateGroupType a, StateGroupType b) {
              if (StateGroupType.Invalid == a) return -1;
              else if (StateGroupType.Invalid == b) return 1;
              else return a.ToString().CompareTo(b.ToString());
            });
          }

          return _GameStateGroups;
        }

        public List<Anki.Cozmo.Audio.GenericState> GetGameStates(Anki.Cozmo.Audio.StateGroupType stateGroup) {
          if (null == _GameStateTypes) {
            _GameStateTypes = new Dictionary<StateGroupType, List<GenericState>>();
            // FIXME This a temp solution to add group types
            List<Anki.Cozmo.Audio.GenericState> musicStates = Enum.GetValues(typeof(Anki.Cozmo.Audio.MUSIC)).Cast<Anki.Cozmo.Audio.GenericState>().ToList();
            _GameStateTypes.Add(StateGroupType.MUSIC, musicStates);
          }

          List<Anki.Cozmo.Audio.GenericState> groupStates;
          if (_GameStateTypes.TryGetValue(stateGroup, out groupStates)) {
            return groupStates;
          }
            
          return null;
        }

        public List<Anki.Cozmo.Audio.SwitchGroupType> GetSwitchStateGroups() {
          if (null == _SwitchStateGroups) {
            _SwitchStateGroups = Enum.GetValues(typeof(Anki.Cozmo.Audio.SwitchGroupType)).Cast<Anki.Cozmo.Audio.SwitchGroupType>().ToList();
            _SwitchStateGroups.Sort(delegate (SwitchGroupType a, SwitchGroupType b) {
              if (SwitchGroupType.Invalid == a) return -1;
              else if (SwitchGroupType.Invalid == b) return 1;
              else return a.ToString().CompareTo(b.ToString());
            });
          }

          return _SwitchStateGroups;
        }

        public List<Anki.Cozmo.Audio.GenericSwitch> GetSwitchStates(Anki.Cozmo.Audio.SwitchGroupType stateGroup) {
          if (null == _SwitchStateTypes) {
            _SwitchStateTypes = new Dictionary<SwitchGroupType, List<GenericSwitch>>();
          }

          List<Anki.Cozmo.Audio.GenericSwitch> groupStates;
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

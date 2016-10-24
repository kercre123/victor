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

      public class UnityAudioClient {

        public event CallbackHandler OnAudioCallback;

        private static String kAudioLogChannelName = "Audio";
        private static UnityAudioClient _AudioClient = null;
        private RobotEngineManager _RobotEngineManager = null;
        private bool _IsInitialized = false;

        public static UnityAudioClient Instance {
          get {
            if (_AudioClient == null) {
              _AudioClient = new UnityAudioClient();
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
            _RobotEngineManager.AddCallback<Anki.Cozmo.Audio.AudioCallback>(HandleCallback);
            _IsInitialized = true;
          }
          else {
            DAS.Warn("UnityAudioClient.Initialize", "Failed to Initialize!");
          }
        }

        ~UnityAudioClient() {
          _RobotEngineManager.RemoveCallback<Anki.Cozmo.Audio.AudioCallback>(HandleCallback);
          _RobotEngineManager = null;
          _IsInitialized = false;
        }

        // Basic Audio Client Operations
        // Return PlayId - Note: PlayId is not guaranteed to be unique it will eventually roll over.
        public ushort PostEvent(GameEvent.GenericEvent audioEvent,
                                GameObjectType gameObject,
                                AudioCallbackFlag callbackFlag = AudioCallbackFlag.EventNone,
                                CallbackHandler handler = null) {
          DAS.Ch_Info(kAudioLogChannelName,
                      "UnityAudioClient.PostAudioEvent",
                      string.Format("Event: {0} Id: {1}", audioEvent, (uint)audioEvent));
          ushort playId = _GetPlayId();

          // Only register for callbacks if a flag is set.
          // Callbacks are only registered if callbackId != kInvalidPlayId
          ushort callbackId = AudioCallbackFlag.EventNone == callbackFlag ? kInvalidPlayId : playId;
          PostAudioEvent msg = new PostAudioEvent(audioEvent, gameObject, callbackId);
          _RobotEngineManager.Message.PostAudioEvent = msg;
          _RobotEngineManager.SendMessage();

          // Assert if a callback handle is passed in and callback flag is set to EventNone
          sysDebug.Assert(!(AudioCallbackFlag.EventNone == callbackFlag && null != handler));
          if (null != handler) {
            AddCallbackHandler(playId, callbackFlag, handler);
          }

          return playId;
        }

        // Pass in game object type to stop audio events on that game object, use Invalid to stop all audio
        public void StopAllAudioEvents(Anki.Cozmo.Audio.GameObjectType gameObject = Anki.Cozmo.Audio.GameObjectType.Invalid) {
          DAS.Ch_Info(kAudioLogChannelName,
                      "UnityAudioClient.StopAllAudioEvents",
                      string.Format("GameObj: {0}", gameObject));
          StopAllAudioEvents msg = new Anki.Cozmo.Audio.StopAllAudioEvents(gameObject);
          _RobotEngineManager.Message.StopAllAudioEvents = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostGameState(GameState.StateGroupType gameStateGroup,
                                  GameState.GenericState gameState) {
          DAS.Ch_Info(kAudioLogChannelName,
                      "UnityAudioClient.PostAudioGameState",
                      string.Format("GameStateGroup: {0} GameState: {1}", gameStateGroup, gameState));
          PostAudioGameState msg = new PostAudioGameState(gameStateGroup, gameState);
          _RobotEngineManager.Message.PostAudioGameState = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostSwitchState(SwitchState.SwitchGroupType switchStateGroup, SwitchState.GenericSwitch switchState, GameObjectType gameObject) {
          DAS.Ch_Info(kAudioLogChannelName,
                      "UnityAudioClient.PostAudioSwitchState",
                      string.Format("SwitchStateGroup: {0} SwitchState: {1} GameObj: {2}", switchStateGroup, switchState, gameObject));
          PostAudioSwitchState msg = new PostAudioSwitchState(switchStateGroup, switchState, gameObject);
          _RobotEngineManager.Message.PostAudioSwitchState = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostParameter(GameParameter.ParameterType parameter,
                                  float parameterValue,
                                  GameObjectType gameObject,
                                  int timeInMilliSeconds = 0,
                                  Anki.Cozmo.Audio.CurveType curve = CurveType.Linear) {
          DAS.Ch_Info(kAudioLogChannelName,
                      "UnityAudioClient.PostAudioParameter",
                      string.Format("Parameter: {0} Id: {1} val: {2}",
                                    parameter, (uint)parameter, parameterValue));
          PostAudioParameter msg = new PostAudioParameter(parameter, parameterValue, gameObject, timeInMilliSeconds, curve);
          _RobotEngineManager.Message.PostAudioParameter = msg;
          _RobotEngineManager.SendMessage();
        }

        public void PostMusicState(GameState.GenericState musicState,
                                   bool interrupt = false,
                                   uint minDurationInMilliSeconds = 0) {
          DAS.Ch_Info(kAudioLogChannelName,
                      "UnityAudioClient.PostAudioMusicState",
                      string.Format("MusicState: {0} Id: {1}",
                                    musicState, (uint)musicState));
          PostAudioMusicState msg = new PostAudioMusicState(musicState, interrupt, minDurationInMilliSeconds);
          _RobotEngineManager.Message.PostAudioMusicState = msg;
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
          DAS.Ch_Debug(kAudioLogChannelName,
                       "UnityAudioClient.AddCallbackHandler",
                       string.Format("Add Callback PlayId: {0} Flags: {1}", playId, flags));
        }

        public void UnregisterCallbackHandler(ushort playId) {
          bool success = _callbackDelegates.Remove(playId);
          if (success) {
            DAS.Ch_Debug(kAudioLogChannelName,
                         "UnityAudioClient.UnregisterCallbackHandler",
                         string.Format("Remove Callback PlayId: {0}", playId));
          }
          else {
            DAS.Warn("UnityAudioClient.UnregisterCallbackHandler", "Failed to Remove Callback Bundle with PlayId " + playId.ToString());
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
            if (null == unregisterHandle) {
              if ((AudioCallbackFlag.EventComplete & callbackType) == AudioCallbackFlag.EventComplete ||
                  (AudioCallbackFlag.EventError & callbackType) == AudioCallbackFlag.EventError) {
                UnregisterCallbackHandler(playId);
              }
            }
            else if (unregisterHandle.Value) {
              UnregisterCallbackHandler(playId);
            }
          }
        }

        // Handle message types
        private void HandleCallback(AudioCallback message) {
          DAS.Ch_Debug(kAudioLogChannelName,
                       "UnityAudioClient.HandleCallback",
                       string.Format("Received Callback message: {0}", message));
          CallbackInfo info = new CallbackInfo(message);
          if (null != OnAudioCallback) {
            OnAudioCallback(info);
          }
          // Call back handle
          PerformCallbackHandler(info.PlayId, info);
        }

        // Testing Helpers
        public static void PostTestTone (float freqencyParameter) {
          Instance.PostParameter(GameParameter.ParameterType.Dev_Tone_Freq, freqencyParameter, GameObjectType.Default);
          Instance.PostEvent(GameEvent.GenericEvent.Play__Dev_Device__Tone_Generator, GameObjectType.Default);
        }


        // Data Helpers
        public class StateMap<GroupType, StateType>
          where GroupType : struct, IConvertible, IComparable, IFormattable 
          where StateType : struct, IConvertible, IComparable, IFormattable
        {
          private readonly List<StateMapGroup<GroupType, StateType>> _Groups = new List<StateMapGroup<GroupType, StateType>>();
          private readonly List<string> _CachedGroupNames = new List<string>();
          private readonly string _BaseEnumPath;

          public StateMap(string baseEnumPath) {
            _BaseEnumPath = baseEnumPath;
            BuildGroupList();
          }

          public List<string> GetGroupNames() {
            return _CachedGroupNames;
          }

          public StateMapGroup<GroupType, StateType> GetGroupByIndex(int index) {
            if (index < 0 || index >= _Groups.Count) {
              return null;
            }

            return _Groups[index];
          }

          private void BuildGroupList() {
            _Groups.Clear();
            _CachedGroupNames.Clear();

            var values = Enum.GetValues(typeof(GroupType));

            foreach (var value in values) {
              var group = new StateMapGroup<GroupType, StateType>((GroupType)value, _BaseEnumPath);
              _Groups.Add(group);
            }

            // sort by name
            _Groups.Sort((StateMapGroup<GroupType, StateType> a, StateMapGroup<GroupType, StateType> b) => {
              if (a.Name == "Invalid") return -1;
              if (a.Name == "Invalid") return 1;
              return a.Name.CompareTo(b.Name);
            });

            foreach (var group in _Groups) {
              _CachedGroupNames.Add(group.Name);
            }
          }
        }

        public class StateMapGroup<GroupType, StateType> 
          where GroupType : struct, IConvertible, IComparable, IFormattable 
          where StateType : struct, IConvertible, IComparable, IFormattable
        {
          public string Name { get; private set; }
          public GroupType Value { get; private set; }

          private readonly Dictionary<string, StateType> _States = new Dictionary<string, StateType>();
          private readonly List<string> _CachedStateNames = new List<string>();
          private string _BaseEnumPath;

          public StateMapGroup(GroupType value, string baseEnumPath) {
            Name = value.ToString();
            Value = value;
            _BaseEnumPath = baseEnumPath;
            BuildStateList();
          }

          public List<string> GetStateNames() {
            return _CachedStateNames;
          }

          public StateType GetStateByIndex(int index) {
            string name = _CachedStateNames[index];

            StateType state;

            if (_States.TryGetValue(name, out state)) {
              return state;
            }

            return (StateType)(object)0; // HACK: We know 0 == Invalid
          }

          private void BuildStateList() {
            _States.Clear();
            _CachedStateNames.Clear();

            var stateEnum = GetStateEnumType(Value, _BaseEnumPath);

            if (stateEnum != null) {
              var names = Enum.GetNames(stateEnum);
              var values = Enum.GetValues(stateEnum).Cast<StateType>().ToList();

              for (int i = 0; i < names.Length; ++i) {
                _States.Add(names[i], values[i]);
                _CachedStateNames.Add(names[i]);
              }

              // sort by name
              _CachedStateNames.Sort ((string a, string b) => {
                if (a == "Invalid") return -1;
                if (b == "Invalid") return 1;
                return a.CompareTo(b);
              });
            }
          }

          private static System.Type GetStateEnumType(GroupType stateGroup, string baseEnumPath) {
            if (stateGroup.ToString() == "Invalid") { // HACK: We know Invalid will always exist
              return null;
            }

            var enumTypeName = baseEnumPath + "." + stateGroup.ToString();
            try {
              var enumType = System.Type.GetType(enumTypeName);
              return enumType;
            }
            catch (Exception e) {
              DAS.Error("UnityAudioClient.GameStateMap.Group.GetStateEnumType", e);
              return null;
            }
          }
        }

        private List<GameObjectType> _GameObjects;
        private List<GameEvent.GenericEvent> _Events;
        private StateMap<GameState.StateGroupType, GameState.GenericState> _GameStateMap = new StateMap<GameState.StateGroupType, GameState.GenericState>("Anki.Cozmo.Audio.GameState");
        private StateMap<SwitchState.SwitchGroupType, SwitchState.GenericSwitch> _SwitchStateMap = new StateMap<Anki.Cozmo.Audio.SwitchState.SwitchGroupType, Anki.Cozmo.Audio.SwitchState.GenericSwitch>("Anki.Cozmo.Audio.SwitchState");
        private List<GameParameter.ParameterType> _RTPCParameters;


        public List<Anki.Cozmo.Audio.GameObjectType> GetGameObjects() {
          if (null == _GameObjects) {
            _GameObjects = Enum.GetValues(typeof(Anki.Cozmo.Audio.GameObjectType)).Cast<Anki.Cozmo.Audio.GameObjectType>().ToList();
          }
          return _GameObjects;
        }

        public List<Anki.Cozmo.Audio.GameEvent.GenericEvent> GetEvents() {
          if (null == _Events) {
            _Events = Enum.GetValues(typeof(Anki.Cozmo.Audio.GameEvent.GenericEvent)).Cast<Anki.Cozmo.Audio.GameEvent.GenericEvent>().ToList();
            _Events.Sort(delegate (GameEvent.GenericEvent a, GameEvent.GenericEvent b) {
              if (GameEvent.GenericEvent.Invalid == a) return -1;
              else if (GameEvent.GenericEvent.Invalid == b) return 1;
              else return a.ToString().CompareTo(b.ToString());
            });
          }

          return _Events;
        }

        public List<string> GetGameStateGroupNames() {
          return _GameStateMap.GetGroupNames();
        }

        public StateMapGroup<GameState.StateGroupType, GameState.GenericState> GetGameStateGroupByIndex(int index) {
          var group = _GameStateMap.GetGroupByIndex(index);
          return group;
        }

        public List<string> GetSwitchGroupNames() {
          return _SwitchStateMap.GetGroupNames();
        }

        public StateMapGroup<SwitchState.SwitchGroupType, SwitchState.GenericSwitch> GetSwitchGroupByIndex(int index) {
          var group = _SwitchStateMap.GetGroupByIndex(index);
          return group;
        }

        public List<GameParameter.ParameterType> GetParameters() {
          if (null == _RTPCParameters) {
            _RTPCParameters = Enum.GetValues(typeof(GameParameter.ParameterType)).Cast<GameParameter.ParameterType>().ToList();
            _RTPCParameters.Sort(delegate (GameParameter.ParameterType a, GameParameter.ParameterType b) {
              if (GameParameter.ParameterType.Invalid == a) return -1;
              else if (GameParameter.ParameterType.Invalid == b) return 1;
              else return a.ToString().CompareTo(b.ToString());
            });
          }

          return _RTPCParameters;
        }

      }
    }
  }
}

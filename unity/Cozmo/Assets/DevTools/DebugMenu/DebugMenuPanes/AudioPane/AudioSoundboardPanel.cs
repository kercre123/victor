using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;


namespace Anki {
  namespace Cozmo {
    namespace Audio {
      
      public class AudioSoundboardPanel : MonoBehaviour {

        // Dropdowns
        [SerializeField]
        private Dropdown _GameObjectDropdown;
        [SerializeField]
        private Dropdown _EventDropdown;
        [SerializeField]
        private Dropdown _GameStateGroupDropdown;
        [SerializeField]
        private Dropdown _GameStateTypeDropdown;
        [SerializeField]
        private Dropdown _SwitchStateGroupDropdown;
        [SerializeField]
        private Dropdown _SwitchStateTypeDropdown;
        [SerializeField]
        private Dropdown _ParameterDropdown;

        // Buttons
        [SerializeField]
        private Button _PostAllButton;
        [SerializeField]
        private Button _StopAllButton;
        [SerializeField]
        private Button _PostEventButton;
        [SerializeField]
        private Button _PostGameStateButton;
        [SerializeField]
        private Button _PostSwitchStateButton;
        [SerializeField]
        private Button _PostParameterButton;

        [SerializeField]
        private Text _AudioLog;

        [SerializeField]
        private Slider _RTPCSlider;
        [SerializeField]
        private Text _RTPCMinText;
        [SerializeField]
        private Text _RTPCMaxText;
        [SerializeField]
        private Text _RTPCValueText;

        private UnityAudioClient _audioClient = UnityAudioClient.Instance;
        private string _LogString = "";

        // Use this for initialization
        void Start() {

          _SetupStaticDropdowns();

          // Setup buttons
          _PostAllButton.onClick.AddListener(() => {
            _PostSwitchState();
            _PostRTPCParameter();
            _PostEvent();
          });
          _StopAllButton.onClick.AddListener(_StopAllEvents);
          _PostEventButton.onClick.AddListener(_PostEvent);
          _PostGameStateButton.onClick.AddListener(_PostGameState);
          _PostSwitchStateButton.onClick.AddListener(_PostSwitchState);
          _PostParameterButton.onClick.AddListener(_PostRTPCParameter);

          // Setup Parameter RTPC Slider
          _RTPCSlider.onValueChanged.AddListener((float value) => {
            _RTPCValueText.text = value.ToString("n2");
          });
          _SetupRTPCSlider(0);
        }

        private void _SetupStaticDropdowns() {

          // Game Objects
          foreach (Anki.Cozmo.Audio.GameObjectType anEnum in _audioClient.GetGameObjects()) {
            _GameObjectDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
          }

          // Events
          foreach (Anki.Cozmo.Audio.GameEvent.GenericEvent anEnum in _audioClient.GetEvents()) {
            _EventDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
          }

          // Game State Groups
          foreach (GameState.StateGroupType anEnum in _audioClient.GetGameStateGroups()) {
            _GameStateGroupDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
          }
          _GameStateGroupDropdown.onValueChanged.AddListener(_SetupGameStateTypeDropdown);

          // Switch State Groups
          foreach (SwitchState.SwitchGroupType anEnum in _audioClient.GetSwitchStateGroups()) {
            _SwitchStateGroupDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
          }
          _SwitchStateGroupDropdown.onValueChanged.AddListener(_SetupSwitchStateTypeDropdown);

          // RTPC Parameters
          foreach (GameParameter.ParameterType anEnum in _audioClient.GetParameters()) {
            _ParameterDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
          }
          _ParameterDropdown.onValueChanged.AddListener( (int value) => {
            GameParameter.ParameterType parameter = _audioClient.GetParameters()[_ParameterDropdown.value];
            _SetupRTPCSlider(parameter);
          });
        }

        // Update State Type Dropdowns
        private void _SetupGameStateTypeDropdown(int value) {
          // Reset Dropdown to show Group specific types
          _GameStateTypeDropdown.options.Clear();
          GameState.StateGroupType stateGroup = _audioClient.GetGameStateGroups()[value];
          if (GameState.StateGroupType.Invalid != stateGroup) {
            List<GameState.GenericState> states = _audioClient.GetGameStates(stateGroup);
            if (null != states) {
              foreach (GameState.GenericState anEnum in states) {
                _GameStateTypeDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
              }
            }
          }
        }

        private void _SetupSwitchStateTypeDropdown(int value) {
          // Reset Dropdown to show Group specific types
          _SwitchStateTypeDropdown.options.Clear();
          SwitchState.SwitchGroupType stateGroup = _audioClient.GetSwitchStateGroups()[value];
          if (SwitchState.SwitchGroupType.Invalid != stateGroup) {
            List<SwitchState.GenericSwitch> states = _audioClient.GetSwitchStates(stateGroup);
            if (null != states) {
              foreach (SwitchState.GenericSwitch anEnum in states) {
                _SwitchStateTypeDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
              }
            }
          }
        }

        private void _SetupRTPCSlider(GameParameter.ParameterType parameter) {
          // Reset slider to show RTPC meta data
          float min = 0.0f;
          float max = 0.0f;
          float rtpcValue = 0.0f;
          if (GameParameter.ParameterType.Invalid != parameter) {
            // Set Parameter data
            // TODO Need meta data to set Min, Max & Default
            min = -1.0f;
            max = 1.0f;
            rtpcValue = 0.0f;
          }
          _RTPCSlider.minValue = min;
          _RTPCSlider.maxValue = max;
          _RTPCSlider.value = rtpcValue;
          _RTPCMinText.text = min.ToString("n2");
          _RTPCMaxText.text = max.ToString("n2");
          _RTPCValueText.text = rtpcValue.ToString("n2");
        }

        // Helpers
        private void _AppendLogEvent(string logEvent) {
          DateTime localDate = DateTime.Now;
          _LogString += localDate.ToString("hh:mm:ss.ffff") + " - " + logEvent + "\n";
          _AudioLog.text = _LogString;
        }


        // Proform Audio Client operations
        private void _PostEvent() {
          Anki.Cozmo.Audio.GameEvent.GenericEvent selectedEvent = _audioClient.GetEvents()[_EventDropdown.value];
          Anki.Cozmo.Audio.GameObjectType selectedGameObj = _audioClient.GetGameObjects()[_GameObjectDropdown.value];
          _audioClient.PostEvent(selectedEvent, selectedGameObj, AudioCallbackFlag.EventAll, (CallbackInfo info) => {
            string log = "Callback Event PlayId: " + info.PlayId + " Type: " + info.CallbackType.ToString();
            _AppendLogEvent(log);
          });
          _AppendLogEvent("Post Event: " + selectedEvent.ToString() + " GameObj: " + selectedGameObj.ToString());
        }

        private void _StopAllEvents() {
          Anki.Cozmo.Audio.GameObjectType selectedGameObj = _audioClient.GetGameObjects()[_GameObjectDropdown.value];
          _audioClient.StopAllAudioEvents(selectedGameObj);
          _AppendLogEvent("Stop All GameObj GameObj: " + selectedGameObj.ToString());
        }

        private void _PostGameState() {
          GameState.StateGroupType groupType = _audioClient.GetGameStateGroups()[_GameStateGroupDropdown.value];
          GameState.GenericState stateType = _audioClient.GetGameStates(groupType)[_GameStateTypeDropdown.value];
          _audioClient.PostGameState(groupType, stateType);
          _AppendLogEvent("Post Game State: " + groupType.ToString() + " : " + stateType.ToString());
        }

        private void _PostSwitchState() {
          SwitchState.SwitchGroupType groupType = _audioClient.GetSwitchStateGroups()[_SwitchStateGroupDropdown.value];
          SwitchState.GenericSwitch stateType = SwitchState.GenericSwitch.Invalid;
          Anki.Cozmo.Audio.GameObjectType selectedGameObj = _audioClient.GetGameObjects()[_GameObjectDropdown.value];

          // TODO: Add PostGameState call
          _AppendLogEvent("Post Switch State: " + groupType.ToString() + " : " + stateType.ToString() + " GameObj: " + selectedGameObj.ToString());
        }

        private void _PostRTPCParameter() {
          GameParameter.ParameterType parameterType = _audioClient.GetParameters()[_ParameterDropdown.value];
          float sliderValue = _RTPCSlider.value;
          Anki.Cozmo.Audio.GameObjectType selectedGameObj = _audioClient.GetGameObjects()[_GameObjectDropdown.value];
          _audioClient.PostParameter(parameterType, sliderValue, selectedGameObj);
          _AppendLogEvent("Post RTPC: " + parameterType.ToString() + " : " + sliderValue.ToString() + " GameObj: " + selectedGameObj.ToString());
        }
      }


    }
  }
}

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

        private AudioClient _audioClient = AudioClient.Instance;
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
          foreach (Anki.Cozmo.Audio.EventType anEnum in _audioClient.GetEvents()) {
            _EventDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
          }

          // Game State Groups
          foreach (Anki.Cozmo.Audio.GameStateGroupType anEnum in _audioClient.GetGameStateGroups()) {
            _GameStateGroupDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
          }
          _GameStateGroupDropdown.onValueChanged.AddListener(_SetupGameStateTypeDropdown);

          // Switch State Groups
          foreach (Anki.Cozmo.Audio.SwitchStateGroupType anEnum in _audioClient.GetSwitchStateGroups()) {
            _SwitchStateGroupDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
          }
          _SwitchStateGroupDropdown.onValueChanged.AddListener(_SetupSwitchStateTypeDropdown);

          // RTPC Parameters
          foreach (Anki.Cozmo.Audio.ParameterType anEnum in _audioClient.GetParameters()) {
            _ParameterDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
          }
          _ParameterDropdown.onValueChanged.AddListener( (int value) => {
            ParameterType parameter = _audioClient.GetParameters()[_ParameterDropdown.value];
            _SetupRTPCSlider(parameter);
          });
        }

        // Update State Type Dropdowns
        private void _SetupGameStateTypeDropdown(int value) {
          // Reset Dropdown to show Group specific types
          _GameStateTypeDropdown.options.Clear();
          GameStateGroupType stateGroup = _audioClient.GetGameStateGroups()[value];
          if (GameStateGroupType.Invalid != stateGroup) {
            List<GameStateType> states = _audioClient.GetGameStates(stateGroup);
            if (null != states) {
              foreach (Anki.Cozmo.Audio.GameStateType anEnum in states) {
                _GameStateTypeDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
              }
            }
          }
        }

        private void _SetupSwitchStateTypeDropdown(int value) {
          // Reset Dropdown to show Group specific types
          _SwitchStateTypeDropdown.options.Clear();
          SwitchStateGroupType stateGroup = _audioClient.GetSwitchStateGroups()[value];
          if (SwitchStateGroupType.Invalid != stateGroup) {
            List<SwitchStateType> states = _audioClient.GetSwitchStates(stateGroup);
            if (null != states) {
              foreach (Anki.Cozmo.Audio.SwitchStateType anEnum in states) {
                _SwitchStateTypeDropdown.options.Add(new Dropdown.OptionData(anEnum.ToString()));
              }
            }
          }
        }

        private void _SetupRTPCSlider(ParameterType parameter) {
          // Reset slider to show RTPC meta data
          float min = 0.0f;
          float max = 0.0f;
          float rtpcValue = 0.0f;
          if (ParameterType.Invalid != parameter) {
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
          Anki.Cozmo.Audio.EventType selectedEvent = _audioClient.GetEvents()[_EventDropdown.value];
          Anki.Cozmo.Audio.GameObjectType selectedGameObj = _audioClient.GetGameObjects()[_GameObjectDropdown.value];
          _audioClient.PostEvent(selectedEvent, selectedGameObj, AudioCallbackFlag.EventAll, (CallbackInfo info) => {
            string log = "Callback Event PlayId: " + info.PlayId + " Type: " + info.CallbackType.ToString();
            _AppendLogEvent(log);
          });
          _AppendLogEvent("Post Event: " + selectedEvent.ToString() + " GameObj: " + selectedGameObj.ToString());
        }

        private void _PostGameState() {
          GameStateGroupType groupType = _audioClient.GetGameStateGroups()[_GameStateGroupDropdown.value];
          GameStateType stateType = GameStateType.Invalid;
          Anki.Cozmo.Audio.GameObjectType selectedGameObj = _audioClient.GetGameObjects()[_GameObjectDropdown.value];

          // TODO: Add PostGameState call
          _AppendLogEvent("Post Game State: " + groupType.ToString() + " : " + stateType.ToString() + " GameObj: " + selectedGameObj.ToString());
        }

        private void _PostSwitchState() {
          SwitchStateGroupType groupType = _audioClient.GetSwitchStateGroups()[_SwitchStateGroupDropdown.value];
          SwitchStateType stateType = SwitchStateType.Invalid;
          Anki.Cozmo.Audio.GameObjectType selectedGameObj = _audioClient.GetGameObjects()[_GameObjectDropdown.value];

          // TODO: Add PostGameState call
          _AppendLogEvent("Post Switch State: " + groupType.ToString() + " : " + stateType.ToString() + " GameObj: " + selectedGameObj.ToString());
        }

        private void _PostRTPCParameter() {
          ParameterType parameterType = _audioClient.GetParameters()[_ParameterDropdown.value];
          float sliderValue = _RTPCSlider.value;
          Anki.Cozmo.Audio.GameObjectType selectedGameObj = _audioClient.GetGameObjects()[_GameObjectDropdown.value];

          // TODO: Add PostGameState call
          _AppendLogEvent("Post RTPC: " + parameterType.ToString() + " : " + sliderValue.ToString() + " GameObj: " + selectedGameObj.ToString());
        }
      }


    }
  }
}

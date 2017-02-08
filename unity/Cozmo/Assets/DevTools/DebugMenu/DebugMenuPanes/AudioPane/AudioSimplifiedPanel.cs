using UnityEngine;
using System.Collections.Generic;
using System;
using UnityEngine.UI;
using System.Linq;


namespace Anki.Cozmo.Audio {
  public class AudioSimplifiedPanel : MonoBehaviour {

    [SerializeField]
    private Dropdown _EventGroupDropdown;
    [SerializeField]
    private Dropdown _EventDropdown;

    [SerializeField]
    private Button _PlayButton;
    [SerializeField]
    private Button _StopButton;

    private static readonly Dictionary<string, Type> _EventTypeDictionary =
      new Dictionary<string, Type> {
      { "Ui", typeof(Anki.AudioMetaData.GameEvent.Ui) },
      { "Sfx", typeof(Anki.AudioMetaData.GameEvent.Sfx) },
      // Leave out VO for now, since we don't have any VO
      //{ "VO", typeof(GameEvent.GenericEvent) },
      { "Music", typeof(Anki.AudioMetaData.GameState.Music) }
    };
      
    private Type _EventType;

    private void Awake() {
      _EventGroupDropdown.options = _EventTypeDictionary.Keys.Select(x => new Dropdown.OptionData(x)).ToList();

      _EventGroupDropdown.value = 0;
      HandleGroupChanged(0);
      _EventGroupDropdown.onValueChanged.AddListener(HandleGroupChanged);
      _PlayButton.onClick.AddListener(HandlePlayClicked);
      _StopButton.onClick.AddListener(HandleStopClicked);
    }

    private void HandleGroupChanged(int val) {
      var option = _EventGroupDropdown.options[val];
      _EventType = _EventTypeDictionary[option.text];

      var eventList = Enum.GetNames(_EventType).ToList();
      eventList.Sort();
      _EventDropdown.options = eventList.Select(x => new Dropdown.OptionData(x)).ToList();
    }

    private void HandlePlayClicked() {
      var group = _EventGroupDropdown.options[_EventGroupDropdown.value];

      if (_EventType == null) {
        return;
      }

      var evt = _EventDropdown.options[_EventDropdown.value];

      object value;
      try {
        value = Enum.Parse(_EventType, evt.text);
      }
      catch {
        return;
      }

      DAS.Debug(this, "Playing Debug Sound " + value);

      switch(group.text){
      case "Music":
        GameAudioClient.SetMusicState((Anki.AudioMetaData.GameState.Music)value);
        break;
      case "Ui":
        GameAudioClient.PostUIEvent((Anki.AudioMetaData.GameEvent.Ui)value);
        break;
      case "Sfx":
        GameAudioClient.PostSFXEvent((Anki.AudioMetaData.GameEvent.Sfx)value);
        break;
      case "Vo":
        GameAudioClient.PostAnnouncerVOEvent((Anki.AudioMetaData.GameEvent.GenericEvent)value);
        break;
      }
    }

    private void HandleStopClicked() {
      var group = _EventGroupDropdown.options[_EventGroupDropdown.value];

      switch(group.text){
      case "Music":
        GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Silent);
        break;
      case "Ui":
        UnityAudioClient.Instance.StopAllAudioEvents(Anki.AudioMetaData.GameObjectType.UI);
        break;
      case "Sfx":
        UnityAudioClient.Instance.StopAllAudioEvents(Anki.AudioMetaData.GameObjectType.SFX);
        break;
      case "Vo":
        UnityAudioClient.Instance.StopAllAudioEvents(Anki.AudioMetaData.GameObjectType.Default);
        break;
      }
    }

  }
}

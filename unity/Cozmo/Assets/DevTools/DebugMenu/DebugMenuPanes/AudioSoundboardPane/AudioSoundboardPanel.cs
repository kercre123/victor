using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

using Anki.Cozmo.Audio;

public class AudioSoundboardPanel : MonoBehaviour {

  [SerializeField]
  private Dropdown _EventDropdown;

  [SerializeField]
  private Button _EventPlayButton;



  private AudioClient _audioClient = AudioClient.Instance;

  private List<Anki.Cozmo.Audio.EventType> _EventEnumList = new List<Anki.Cozmo.Audio.EventType>();

	// Use this for initialization
	void Start () {

    // Create Event Map & populate event dropdown
    foreach (Anki.Cozmo.Audio.EventType anEvent in Enum.GetValues(typeof(Anki.Cozmo.Audio.EventType))) {
      _EventEnumList.Add(anEvent);
      _EventDropdown.options.Add(new Dropdown.OptionData(anEvent.ToString()));
    }
    // Setup Event button
    _EventPlayButton.onClick.AddListener( () => {
      Anki.Cozmo.Audio.EventType selectedEvent = _EventEnumList[_EventDropdown.value];
      _audioClient.PostEvent(selectedEvent, 0);
      DAS.Event("AudioSoundboardPanel._EventPlayButton.onClick",
        "Attempt to post audio event: " + selectedEvent.ToString());
    });
	}
	
	// Update is called once per frame
	void Update () {
	
	}
}

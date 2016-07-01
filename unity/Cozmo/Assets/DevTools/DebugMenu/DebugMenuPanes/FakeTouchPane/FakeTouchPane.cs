using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using DataPersistence;

namespace Cozmo.UI {
  public class FakeTouchPane : MonoBehaviour {

    [SerializeField]
    private Button _StartPlaybackButton;

    [SerializeField]
    private Button _RecordToggleButton;
    private Text _RecordToggleText;

    [SerializeField]
    private Button _SaveRecordingButton;

    [SerializeField]
    private Button _StartSoakTestButton;

    [SerializeField]
    private Dropdown _RecordingDropdown;

    [SerializeField]
    private InputField _SaveStringInput;

    private List<string> _RecordingNames = new List<string>();


    private void Start() {
      _StartPlaybackButton.onClick.AddListener(HandlePlaybackButtonClicked);
      _RecordToggleButton.onClick.AddListener(HandleRecordToggleClicked);
      _SaveRecordingButton.onClick.AddListener(HandleSaveButtonClicked);
      _StartSoakTestButton.onClick.AddListener(HandleSoakButtonClicked);
      _RecordToggleText = _RecordToggleButton.GetComponentInChildren<Text>();
      _SaveStringInput.text = "Name Recording To Save Here";
      UpdateUI();
    }

    private void OnDestroy() {
      _StartPlaybackButton.onClick.RemoveListener(HandlePlaybackButtonClicked);
      _RecordToggleButton.onClick.RemoveListener(HandleRecordToggleClicked);
      _SaveRecordingButton.onClick.RemoveListener(HandleSaveButtonClicked);
      _StartSoakTestButton.onClick.RemoveListener(HandleSoakButtonClicked);
    }

    private void HandlePlaybackButtonClicked() {
      FakeTouchManager.Instance.PlayBackRecordedInputs(_RecordingNames[_RecordingDropdown.value]);
      UpdateUI();
      DebugMenuManager.Instance.CloseDebugMenuDialog();
    }

    private void HandleRecordToggleClicked() {
      FakeTouchManager.Instance.SetRecordingInputs(!FakeTouchManager.Instance.IsRecordingTouches);
      UpdateUI();
      if (FakeTouchManager.Instance.IsRecordingTouches) {
        DebugMenuManager.Instance.CloseDebugMenuDialog();
      }
    }

    private void UpdateUI() {
      if (FakeTouchManager.Instance.IsRecordingTouches) {
        _RecordToggleText.text = "Stop Recording Touches";
      }
      else {
        _RecordToggleText.text = "Start Recording Touches";
      }
      _RecordingNames.Clear();
      _RecordingDropdown.options.Clear();
      foreach (string key in DataPersistenceManager.Instance.Data.DebugPrefs.FakeTouchRecordings.Keys) {
        Dropdown.OptionData optionData = new Dropdown.OptionData();
        optionData.text = key;
        _RecordingNames.Add(key);
        _RecordingDropdown.options.Add(optionData);
      }
      _RecordingDropdown.RefreshShownValue();
    }

    private void HandleSaveButtonClicked() {
      FakeTouchManager.Instance.SetRecordingInputs(false);
      FakeTouchManager.Instance.SaveRecordedInputs(_SaveStringInput.text);
      UpdateUI();
    }

    private void HandleSoakButtonClicked() {
      DebugMenuManager.Instance.CloseDebugMenuDialog();
      FakeTouchManager.Instance.IsSoakingTouches = true;
    }


  }
}
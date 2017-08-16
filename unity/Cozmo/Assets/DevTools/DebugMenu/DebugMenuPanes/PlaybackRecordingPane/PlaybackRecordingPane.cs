using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using System.Linq;
using DataPersistence;

namespace Cozmo.UI {
  public class PlaybackRecordingPane : MonoBehaviour {

    [SerializeField]
    private Button _RecordToggleButton;
    private Text _RecordToggleText;

    [SerializeField]
    private Dropdown _RecordingDropdown;
    private const string _kFileSelectMessage = "Select File for Playback";

    [SerializeField]
    private Dropdown _ObjOnScreenDropdown;
    private const string _kObjOnScreenMessage = "Select Object to Verify";

    [SerializeField]
    private InputField _SaveStringInput;
    private const string _kSaveStringStartMessage = "Name Recording to Save Here";

    [SerializeField]
    private InputField _NumPlayTimes;
    private const string _kPlayTimesStartMessage = "Number of times to repeat playback";

    //unity doesn't support tuples for some silly reason so this is a hack
    private List<KeyValuePair<UnityEngine.Object, Type>> _ObjsOnScreen =
                    new List<KeyValuePair<UnityEngine.Object, Type>>();
    private List<string> _RecordingNames = new List<string>();

    protected void Awake() {
      _RecordToggleButton.onClick.AddListener(HandleRecordToggleClicked);
      _RecordingDropdown.onValueChanged.AddListener(HandleFileSelected);
      _ObjOnScreenDropdown.onValueChanged.AddListener(HandleVerifySelected);
    }

    private void Start() {
      _RecordToggleText = _RecordToggleButton.GetComponentInChildren<Text>();
      _SaveStringInput.text = _kSaveStringStartMessage;
      _NumPlayTimes.text = _kPlayTimesStartMessage;
      _RecordingNames.Clear();
      _RecordingDropdown.options.Clear();
      //update dropdown of recordings
      Dropdown.OptionData fileMessage = new Dropdown.OptionData();
      fileMessage.text = _kFileSelectMessage;
      _RecordingNames.Add("");
      _RecordingDropdown.options.Add(fileMessage);
      foreach (string file in System.IO.Directory.GetFiles(Application.persistentDataPath, "*.txt")) {
        Dropdown.OptionData TextOptionData = new Dropdown.OptionData();
        TextOptionData.text = file;
        _RecordingNames.Add(file);
        _RecordingDropdown.options.Add(TextOptionData);
      }
      _RecordingDropdown.RefreshShownValue();
      //update dropdown of stuff to verify
      _ObjsOnScreen.Clear();
      _ObjOnScreenDropdown.options.Clear();
      foreach (Type stepType in Anki.Core.UI.Automation.Automation.Instance.SupportedObjectTypes()) {
        var objects = Anki.Core.UI.Automation.Automation.Instance.GetObjects(stepType);
        if (objects != null) {
          foreach (UnityEngine.Object obj in Anki.Core.UI.Automation.Automation.Instance.GetObjects(stepType)) {
            Dropdown.OptionData ObjOptionData = new Dropdown.OptionData();
            ObjOptionData.text = Anki.Core.UI.Automation.Automation.Instance.ShortString(obj, stepType);
            _ObjsOnScreen.Add(new KeyValuePair<UnityEngine.Object, Type>(obj, stepType));
            _ObjOnScreenDropdown.options.Add(ObjOptionData);
          }
        }
      }
      Dropdown.OptionData objMessage = new Dropdown.OptionData();
      objMessage.text = _kObjOnScreenMessage;
      _ObjsOnScreen.Add(new KeyValuePair<UnityEngine.Object, Type>(null, null));
      _ObjOnScreenDropdown.options.Add(objMessage);
      _ObjOnScreenDropdown.RefreshShownValue();
    }

    private void OnDestroy() {
      _RecordToggleButton.onClick.RemoveListener(HandleRecordToggleClicked);
    }

    public void HandleVerifySelected(int i) {
      if (_ObjOnScreenDropdown.captionText.text.Equals(_kObjOnScreenMessage)) {
        return;
      }
      KeyValuePair<UnityEngine.Object, Type> obj = _ObjsOnScreen[_ObjOnScreenDropdown.value];
      Anki.Core.UI.Automation.Automation.Instance.RecordVerifyObject(obj.Key, obj.Value);
    }

    public void HandleFileSelected(int i) {
      if (_RecordingDropdown.captionText.text.Equals(_kFileSelectMessage)) {
        return;
      }
      string filename = _RecordingNames[_RecordingDropdown.value];
      uint numPlayTimes;
      if (!UInt32.TryParse(_NumPlayTimes.GetComponentInChildren<Text>().text, out numPlayTimes)) {
        numPlayTimes = 1;
      }
      Anki.Core.UI.Automation.Automation.Instance.PlayFile(filename, "", numPlayTimes);
      DebugMenuManager.Instance.CloseDebugMenuDialog();
    }

    private void HandleRecordToggleClicked() {
      if (Anki.Core.UI.Automation.Automation.Instance.IsIdle) {
        string fileName = _SaveStringInput.text;
        //We add the date time to the end of the save file if the user did not specify
        //a name so that each save file is unique if doing a quick recording
        if (fileName == _kSaveStringStartMessage) {
          fileName = "Recording_" + System.DateTime.Now.ToString("yyyyMMddHHmmss");
        }
        Anki.Core.UI.Automation.Automation.Instance.RecordFile(Application.persistentDataPath
                                                                 + "/" + fileName + ".txt");
        DebugMenuManager.Instance.CloseDebugMenuDialog();
      }
      else {
        Anki.Core.UI.Automation.Automation.Instance.Stop();
      }
    }

    private void Update() {
      if (Anki.Core.UI.Automation.Automation.Instance.IsIdle) {
        _RecordToggleText.text = "Start Recording";
      }
      else {
        if (Anki.Core.UI.Automation.Automation.Instance.IsRecording) {
          _RecordToggleText.text = "Stop Recording/Save";
        }
        else {
          _RecordToggleText.text = "Stop Playing";
        }
      }
    }

  }
}

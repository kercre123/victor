using UnityEngine;
using System.Collections;

public class FactoryOptionsPanel : MonoBehaviour {

  public System.Action<bool> OnSetSim;
  public System.Action OnOTAButton;
  public System.Action<string> OnConsoleLogFilter;

  public System.Action<bool> OnEnableNVStorageWrites;
  public System.Action<bool> OnCheckPreviousResults;
  public System.Action<bool> OnWipeNVstorageAtStart;
  public System.Action<bool> OnSkipBlockPickup;

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  [SerializeField]
  private UnityEngine.UI.Toggle _SimToggle;

  [SerializeField]
  private UnityEngine.UI.Button _OTAButton;

  [SerializeField]
  public UnityEngine.UI.InputField _LogFilterInput;

  // Dev Toggles

  [SerializeField]
  private UnityEngine.UI.Toggle _EnableNVStorageWrites;

  [SerializeField]
  private UnityEngine.UI.Toggle _CheckPreviousResults;

  [SerializeField]
  private UnityEngine.UI.Toggle _WipeNVStorageAtStart;

  [SerializeField]
  private UnityEngine.UI.Toggle _SkipBlockPickup;

  public void Initialize(bool sim, string logFilter) {

    _SimToggle.isOn = sim;
    _EnableNVStorageWrites.isOn = PlayerPrefs.GetInt("EnableNStorageWritesToggle", 1) == 1;
    _CheckPreviousResults.isOn = PlayerPrefs.GetInt("CheckPreviousResult", 1) == 1;
    _WipeNVStorageAtStart.isOn = PlayerPrefs.GetInt("WipeNVStorageAtStart", 0) == 1;
    _SkipBlockPickup.isOn = PlayerPrefs.GetInt("SkipBlockPickup", 0) == 1;

    _LogFilterInput.text = logFilter;
  }

  // Use this for initialization
  void Start() {
    _CloseButton.onClick.AddListener(() => GameObject.Destroy(gameObject));
    _SimToggle.onValueChanged.AddListener(HandleOnSetSimType);
    _LogFilterInput.onValueChanged.AddListener(HandleLogInputChange);
    _OTAButton.onClick.AddListener(HandleOTAButton);

    _EnableNVStorageWrites.onValueChanged.AddListener(HandleEnableNVStorageWrites);
    _CheckPreviousResults.onValueChanged.AddListener(HandleCheckPreviousResults);
    _WipeNVStorageAtStart.onValueChanged.AddListener(HandleWipeNVStorageAtStart);
    _SkipBlockPickup.onValueChanged.AddListener(HandleSkipBlockPickup);
  }


  void HandleEnableNVStorageWrites(bool toggleValue) {
    if (OnEnableNVStorageWrites != null) {
      OnEnableNVStorageWrites(toggleValue);
    }
  }

  void HandleCheckPreviousResults(bool toggleValue) {
    if (OnCheckPreviousResults != null) {
      OnCheckPreviousResults(toggleValue);
    }
  }

  void HandleWipeNVStorageAtStart(bool toggleValue) {
    if (OnWipeNVstorageAtStart != null) {
      OnWipeNVstorageAtStart(toggleValue);
    }
  }

  void HandleSkipBlockPickup(bool toggleValue) {
    if (OnSkipBlockPickup != null) {
      OnSkipBlockPickup(toggleValue);
    }
  }

  void HandleLogInputChange(string input) {
    if (OnConsoleLogFilter != null) {
      OnConsoleLogFilter(input);
    }
    PlayerPrefs.SetString("LogFilter", input);
    PlayerPrefs.Save();
  }

  void HandleOTAButton() {
    if (OnOTAButton != null) {
      OnOTAButton();
    }
  }

  void HandleOnSetSimType(bool isSim) {
    if (OnSetSim != null) {
      OnSetSim(isSim);
    }
  }
}

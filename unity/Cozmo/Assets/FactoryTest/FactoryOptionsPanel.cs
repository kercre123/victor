using UnityEngine;
using System.Collections;

#if FACTORY_TEST

public class FactoryOptionsPanel : MonoBehaviour {

  public System.Action<int> OnSetTestNumber;
  public System.Action<int> OnSetStationNumber;
  public System.Action<bool> OnSetSim;
  public System.Action OnOTAButton;
  public System.Action<string> OnConsoleLogFilter;

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  [SerializeField]
  public UnityEngine.UI.InputField _StationNumberInput;

  [SerializeField]
  public UnityEngine.UI.InputField _TestNumberInput;

  [SerializeField]
  private UnityEngine.UI.Toggle _SimToggle;

  [SerializeField]
  private UnityEngine.UI.Button _OTAButton;

  [SerializeField]
  public UnityEngine.UI.InputField _LogFilterInput;

  public void Initialize(bool sim, string logFilter) {
    _SimToggle.isOn = sim;
    _LogFilterInput.text = logFilter;
  }

  // Use this for initialization
  void Start() {
    _CloseButton.onClick.AddListener(() => GameObject.Destroy(gameObject));
    _StationNumberInput.onEndEdit.AddListener(HandleOnSetStationNumber);
    _TestNumberInput.onEndEdit.AddListener(HandleOnSetTestNumber);
    _SimToggle.onValueChanged.AddListener(HandleOnSetSimType);
    _LogFilterInput.onValueChanged.AddListener(HandleLogInputChange);
    _OTAButton.onClick.AddListener(HandleOTAButton);
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

  void HandleOnSetTestNumber(string input) {
    if (OnSetTestNumber != null) {
      OnSetTestNumber(int.Parse(input));
    }
  }

  void HandleOnSetStationNumber(string input) {
    if (OnSetStationNumber != null) {
      OnSetStationNumber(int.Parse(input));
    }
  }

  void HandleOnSetSimType(bool isSim) {
    if (OnSetSim != null) {
      OnSetSim(isSim);
    }
  }
}

#endif
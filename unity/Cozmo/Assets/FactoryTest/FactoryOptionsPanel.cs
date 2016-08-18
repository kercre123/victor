using UnityEngine;
using System.Collections;

public class FactoryOptionsPanel : MonoBehaviour {

  public System.Action<bool> OnSetSim;
  public System.Action OnOTAStarted;
  public System.Action OnOTAFinished;

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

  [SerializeField]
  private UnityEngine.UI.Toggle _EnableRobotSound;

  [SerializeField]
  private UnityEngine.UI.Toggle _ConnectToRobotOnly;

  [SerializeField]
  private FactoryOTAPanel _FactoryOTAPanelPrefab;
  private FactoryOTAPanel _FactoryOTAPanelInstance;

  private Canvas _Canvas;
  private PingStatus _PingStatusComponent;

  public void Initialize(bool sim, string logFilter, Canvas canvas, PingStatus pingStatusComponent) {

    _Canvas = canvas;
    _PingStatusComponent = pingStatusComponent;

    _SimToggle.isOn = sim;
    _EnableNVStorageWrites.isOn = PlayerPrefs.GetInt("EnableNStorageWritesToggle", 1) == 1;
    _CheckPreviousResults.isOn = PlayerPrefs.GetInt("CheckPreviousResult", 0) == 1;
    _WipeNVStorageAtStart.isOn = PlayerPrefs.GetInt("WipeNVStorageAtStart", 0) == 1;
    _SkipBlockPickup.isOn = PlayerPrefs.GetInt("SkipBlockPickup", 0) == 1;
    _EnableRobotSound.isOn = PlayerPrefs.GetInt("EnableRobotSound", 1) == 1;
    _ConnectToRobotOnly.isOn = PlayerPrefs.GetInt("ConnectToRobotOnly", 0) == 1;

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
    _EnableRobotSound.onValueChanged.AddListener(HandleEnableRobotSound);
    _ConnectToRobotOnly.onValueChanged.AddListener(HandleConnectToRobotOnly);

    // Disable the options that shouldn't be available in non-dev mode. Using a local variable
    // instead of the constant solves a compiler error about unreacheable code.
    bool isFactoryDevMode = BuildFlags.kIsFactoryDevMode;
    if (!isFactoryDevMode) {
      _SimToggle.gameObject.SetActive(false);
      _LogFilterInput.gameObject.SetActive(false);

      _EnableNVStorageWrites.gameObject.SetActive(false);
      _CheckPreviousResults.gameObject.SetActive(false);
      //_WipeNVStorageAtStart.gameObject.SetActive(false);
      _SkipBlockPickup.gameObject.SetActive(false);
      _EnableRobotSound.gameObject.SetActive(false);
    }
  }

  void Update() {
    bool pingStatus = _PingStatusComponent.GetPingStatus();
    _OTAButton.interactable = pingStatus;
    _OTAButton.image.color = pingStatus ? Color.green : Color.grey;
  }

  void HandleEnableNVStorageWrites(bool toggleValue) {
    PlayerPrefs.SetInt("EnableNStorageWritesToggle", toggleValue ? 1 : 0);
    PlayerPrefs.Save();

    if (toggleValue) {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_EnableNVStorageWrites", "1");
    }
    else {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_EnableNVStorageWrites", "0");
    }
  }

  void HandleCheckPreviousResults(bool toggleValue) {
    PlayerPrefs.SetInt("CheckPreviousResult", toggleValue ? 1 : 0);
    PlayerPrefs.Save();

    if (toggleValue) {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_CheckPrevFixtureResults", "1");
    }
    else {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_CheckPrevFixtureResults", "0");
    }
  }

  void HandleWipeNVStorageAtStart(bool toggleValue) {
    PlayerPrefs.SetInt("WipeNVStorageAtStart", toggleValue ? 1 : 0);
    PlayerPrefs.Save();

    if (toggleValue) {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_WipeNVStorage", "1");
    }
    else {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_WipeNVStorage", "0");
    }
  }

  void HandleSkipBlockPickup(bool toggleValue) {
    PlayerPrefs.SetInt("SkipBlockPickup", toggleValue ? 1 : 0);
    PlayerPrefs.Save();

    if (toggleValue) {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_SkipBlockPickup", "1");
    }
    else {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_SkipBlockPickup", "0");
    }
  }

  void HandleEnableRobotSound(bool toggleValue) {
    PlayerPrefs.SetInt("EnableRobotSound", toggleValue ? 1 : 0);
    PlayerPrefs.Save();
  }

  void HandleConnectToRobotOnly(bool toggleValue) {
    PlayerPrefs.SetInt("ConnectToRobotOnly", toggleValue ? 1 : 0);
    PlayerPrefs.Save();

    if (toggleValue) {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_ConnectToRobotOnly", "1");
    }
    else {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_ConnectToRobotOnly", "0");
    }
  }

  void HandleLogInputChange(string input) {
    ConsoleLogManager.Instance.LogFilter = input;
    PlayerPrefs.SetString("LogFilter", input);
    PlayerPrefs.Save();
  }

  void HandleOnSetSimType(bool isSim) {
    if (OnSetSim != null) {
      OnSetSim(isSim);
    }
  }

  void HandleOTAButton() {
    if (_FactoryOTAPanelInstance != null) {
      GameObject.Destroy(_FactoryOTAPanelInstance);
    }

    _FactoryOTAPanelInstance = GameObject.Instantiate(_FactoryOTAPanelPrefab).GetComponent<FactoryOTAPanel>();
    _FactoryOTAPanelInstance.transform.SetParent(_Canvas.transform, false);
    _FactoryOTAPanelInstance.OnOTAStarted += HandleOTAStarted;
    _FactoryOTAPanelInstance.OnOTAFinished += HandleOTAFinished;
  }

  void HandleOTAStarted() {
    if (OnOTAStarted != null) {
      OnOTAStarted();
    }
  }

  void HandleOTAFinished() {
    if (OnOTAFinished != null) {
      OnOTAFinished();
    }
  }
}

using UnityEngine;
using System.Collections;

#if FACTORY_TEST

public class FactoryOptionsPanel : MonoBehaviour {

  public System.Action<bool> OnSetSim;

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  [SerializeField]
  private UnityEngine.UI.Toggle _SimToggle;

  // Dev Toggles

  [SerializeField]
  private UnityEngine.UI.Toggle _EnableNVStorageWrites;

  [SerializeField]
  private UnityEngine.UI.Toggle _CheckPreviousResults;

  [SerializeField]
  private UnityEngine.UI.Toggle _EnableRobotSound;

  [SerializeField]
  private UnityEngine.UI.Toggle _ConnectToRobotOnly;

  [SerializeField]
  private UnityEngine.UI.Toggle _CheckFirmwareVersion;

  public void Initialize(bool sim, Canvas canvas, PingStatus pingStatusComponent) {
    _SimToggle.isOn = sim;
    _EnableNVStorageWrites.isOn = PlayerPrefs.GetInt("EnableNStorageWritesToggle", 1) == 1;
    _CheckPreviousResults.isOn = PlayerPrefs.GetInt("CheckPreviousResult", 0) == 1;
    _EnableRobotSound.isOn = PlayerPrefs.GetInt("EnableRobotSound", 1) == 1;
    _ConnectToRobotOnly.isOn = PlayerPrefs.GetInt("ConnectToRobotOnly", 0) == 1;
    _CheckFirmwareVersion.isOn = PlayerPrefs.GetInt("CheckFirmwareVersion", 1) == 1;
  }

  // Use this for initialization
  void Start() {
    _CloseButton.onClick.AddListener(() => GameObject.Destroy(gameObject));

    _SimToggle.onValueChanged.AddListener(HandleOnSetSimType);

    _EnableNVStorageWrites.onValueChanged.AddListener(HandleEnableNVStorageWrites);
    _CheckPreviousResults.onValueChanged.AddListener(HandleCheckPreviousResults);
    _EnableRobotSound.onValueChanged.AddListener(HandleEnableRobotSound);
    _ConnectToRobotOnly.onValueChanged.AddListener(HandleConnectToRobotOnly);
    _CheckFirmwareVersion.onValueChanged.AddListener(HandleCheckFirmwareVersion);

    // Disable the options that shouldn't be available in non-dev mode. Using a local variable
    // instead of the constant solves a compiler error about unreacheable code.

#if !FACTORY_TEST_DEV
    _SimToggle.gameObject.SetActive(false);

    _EnableNVStorageWrites.gameObject.SetActive(false);
    _CheckPreviousResults.gameObject.SetActive(false);
    _CheckFirmwareVersion.gameObject.SetActive(false);
#endif
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
  
  void HandleCheckFirmwareVersion(bool toggleValue) {
    PlayerPrefs.SetInt("CheckFirmwareVersion", toggleValue ? 1 : 0);
    PlayerPrefs.Save();

    if (toggleValue) {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_CheckFWVersion", "1");
    }
    else {
      RobotEngineManager.Instance.SetDebugConsoleVar("BFT_CheckFWVersion", "0");
    }
  }

  void HandleOnSetSimType(bool isSim) {
    if (OnSetSim != null) {
      OnSetSim(isSim);
    }
  }
}

#endif
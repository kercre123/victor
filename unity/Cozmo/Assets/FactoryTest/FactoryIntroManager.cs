using UnityEngine;
using System.Collections;
using System.Collections.Generic;

using Anki.Cozmo.ExternalInterface;

public class FactoryIntroManager : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.Image _Background;

  [SerializeField]
  private UnityEngine.UI.Image _RestartOverlay;

  [SerializeField]
  private UnityEngine.UI.Button _OptionsButton;

  [SerializeField]
  private UnityEngine.UI.Button _RestartButton;

  [SerializeField]
  private UnityEngine.UI.Button _StartButton;

  [SerializeField]
  private UnityEngine.UI.Text _PingStatusText;

  [SerializeField]
  private FactoryOptionsPanel _FactoryOptionsPanelPrefab;
  private FactoryOptionsPanel _FactoryOptionsPanelInstance;

  [SerializeField]
  private FactoryOTAPanel _FactoryOTAPanelPrefab;
  private FactoryOTAPanel _FactoryOTAPanelInstance;

  [SerializeField]
  private UnityEngine.UI.Text _StatusText;

  [SerializeField]
  private Canvas _Canvas;

  [SerializeField]
  private UnityEngine.UI.Image _InProgressSpinner;

  private bool _IsSim = false;

  private const int kRobotID = 1;

  void Start() {
    _RestartOverlay.gameObject.SetActive(false);
    DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.SOSLoggerEnabled = true;
    ConsoleLogManager.Instance.LogFilter = PlayerPrefs.GetString("LogFilter");

    SetStatusText("Not Connected");

    RobotEngineManager.Instance.Disconnect();
    RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToClient;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
    RobotEngineManager.Instance.RobotConnected += HandleConnected;
    RobotEngineManager.Instance.OnFactoryResult += FactoryResult;
    _RestartButton.gameObject.SetActive(false);

    _RestartButton.onClick.AddListener(RestartTestApp);
    _OptionsButton.onClick.AddListener(HandleOptionsButtonClick);
    _StartButton.onClick.AddListener(HandleStartButtonClick);

    _InProgressSpinner.gameObject.SetActive(false);

    _OptionsButton.gameObject.SetActive(BuildFlags.kIsFactoryDevMode);
  }

  private void HandleSetSimType(bool isSim) {
    _IsSim = isSim;
  }

  private void HandleStartButtonClick() {
    RobotEngineManager.Instance.Disconnect();
    RobotEngineManager.Instance.Connect("127.0.0.1");

    _StartButton.gameObject.SetActive(false);
    _InProgressSpinner.gameObject.SetActive(true);
  }

  private void HandleConnectedToClient(string connectionIdentifier) {
    RobotEngineManager.Instance.StartEngine();
    RobotEngineManager.Instance.ForceAddRobot(kRobotID, _IsSim ? "127.0.0.1" : "172.31.1.1", _IsSim);

    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues();
  }

  private void HandleConnected(int robotID) {
    SetStatusText("Factory App Connected To Robot");

    HandleEnableNVStorageWrites(PlayerPrefs.GetInt("EnableNStorageWritesToggle", 1) == 1);
    HandleCheckPreviousResults(PlayerPrefs.GetInt("CheckPreviousResult", 1) == 1);
    HandleWipeNVStorageAtStart(PlayerPrefs.GetInt("WipeNVStorageAtStart", 0) == 1);
    HandleSkipBlockPickup(PlayerPrefs.GetInt("SkipBlockPickup", 0) == 1);

    // runs the factory test.
    RobotEngineManager.Instance.CurrentRobot.WakeUp(true);
    RobotEngineManager.Instance.CurrentRobot.EnableReactionaryBehaviors(false);
    RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
    RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.FactoryTest);

    _RestartButton.gameObject.SetActive(true);
  }

  private void HandleOptionsButtonClick() {
    if (_FactoryOptionsPanelInstance != null) {
      GameObject.Destroy(_FactoryOptionsPanelInstance);
    }
    _FactoryOptionsPanelInstance = GameObject.Instantiate(_FactoryOptionsPanelPrefab).GetComponent<FactoryOptionsPanel>();
    _FactoryOptionsPanelInstance.transform.SetParent(_Canvas.transform, false);
    _FactoryOptionsPanelInstance.OnSetSim += HandleSetSimType;
    _FactoryOptionsPanelInstance.OnOTAButton += HandleOTAButton;
    _FactoryOptionsPanelInstance.OnConsoleLogFilter += HandleSetConsoleLogFilter;

    _FactoryOptionsPanelInstance.OnEnableNVStorageWrites += HandleEnableNVStorageWrites;
    _FactoryOptionsPanelInstance.OnCheckPreviousResults += HandleCheckPreviousResults;
    _FactoryOptionsPanelInstance.OnWipeNVstorageAtStart += HandleWipeNVStorageAtStart;
    _FactoryOptionsPanelInstance.OnSkipBlockPickup += HandleSkipBlockPickup;

    _FactoryOptionsPanelInstance.Initialize(_IsSim, ConsoleLogManager.Instance.LogFilter);
  }

  private void HandleSetConsoleLogFilter(string input) {
    ConsoleLogManager.Instance.LogFilter = input;
  }

  private void HandleOTAButton() {
    RobotEngineManager.Instance.Disconnect();
    RobotEngineManager.Instance.Connect("127.0.0.1");

    RobotEngineManager.Instance.RobotConnected -= HandleConnected;
    RobotEngineManager.Instance.RobotConnected += HandleOTAConnected;
    _FactoryOTAPanelInstance = GameObject.Instantiate(_FactoryOTAPanelPrefab).GetComponent<FactoryOTAPanel>();
    _FactoryOTAPanelInstance.transform.SetParent(_Canvas.transform, false);
  }

  private void HandleOTAConnected(int robotID) {
    _FactoryOTAPanelInstance.OnRestartButton += RestartTestApp;
    RobotEngineManager.Instance.UpdateFirmware(0);
  }

  private void SetStatusText(string txt) {
    if (_StatusText != null) {
      _StatusText.text = txt;
    }
  }

  private void HandleDisconnectedFromClient(DisconnectionReason obj) {
    SetStatusText("Disconnected: " + obj.ToString());
    TestFailed();
  }

  private void TestFailed() {
    if (_Background == null) {
      return; // quit case
    }
    _Background.color = Color.red;
    _InProgressSpinner.gameObject.SetActive(false);
    _RestartButton.gameObject.SetActive(true);
    TearDownEngine();
  }

  private void TestPassed() {
    _Background.color = Color.green;
    _InProgressSpinner.gameObject.SetActive(false);
    _RestartButton.gameObject.SetActive(true);
    TearDownEngine();
  }

  private void TearDownEngine() {
    CozmoBinding.Shutdown();
    RobotEngineManager.Instance.ConnectedToClient -= HandleConnectedToClient;
    RobotEngineManager.Instance.DisconnectedFromClient -= HandleDisconnectedFromClient;
    RobotEngineManager.Instance.RobotConnected -= HandleConnected;
    RobotEngineManager.Instance.OnFactoryResult -= FactoryResult;
  }

  private void RestartTestApp() {
    _RestartButton.gameObject.SetActive(false);
    _RestartOverlay.gameObject.SetActive(true);
    UnityEngine.SceneManagement.SceneManager.LoadScene("FactoryTest");
  }

  private void FactoryResult(Anki.Cozmo.FactoryTestResultEntry result) {
    SetStatusText("Result Code: " + (int)result.result + " (" + result.result + ")");
    if (result.result == Anki.Cozmo.FactoryTestResultCode.SUCCESS) {
      TestPassed();
    }
    else {
      TestFailed();
    }
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

  void Update() {
    if (GetComponent<PingStatus>().GetPingStatus() || _IsSim) {
      _StartButton.transform.FindChild("Text").GetComponent<UnityEngine.UI.Text>().text = "START";
      _StartButton.image.color = Color.green;
      _StartButton.interactable = true;
      _PingStatusText.text = "Ping Status: Connected";
    }
    else {
      _StartButton.image.color = Color.gray;
      _StartButton.transform.FindChild("Text").GetComponent<UnityEngine.UI.Text>().text = "NO ROBOT CONNECTED";
      _StartButton.interactable = false;
      _PingStatusText.text = "Ping Status: Not Connected";
    }


  }
}

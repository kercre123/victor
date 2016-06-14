using UnityEngine;
using System.Collections;
using System.Collections.Generic;

using Anki.Cozmo.ExternalInterface;

public class FactoryIntroManager : MonoBehaviour {

  [SerializeField]
  private ConnectDialog _DevConnectDialog;
  private GameObject _DevConnectDialogInstance;

  [SerializeField]
  private UnityEngine.UI.Image _Background;

  [SerializeField]
  private UnityEngine.UI.Image _RestartOverlay;

  [SerializeField]
  private UnityEngine.UI.Button _LogsButton;

  [SerializeField]
  private UnityEngine.UI.Button _OptionsButton;

  [SerializeField]
  private UnityEngine.UI.Button _RestartButton;

  [SerializeField]
  private UnityEngine.UI.Button _StartButton;

  [SerializeField]
  private UnityEngine.UI.Text _PingStatusText;

  [SerializeField]
  private FactoryLogPanel _FactoryLogPanelPrefab;
  private FactoryLogPanel _FactoryLogPanelInstance;

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

  private string _LogFilter = "";
  private bool _IsSim = false;

  private List<string> _LogList = new List<string>();

  void Start() {
    _RestartOverlay.gameObject.SetActive(false);
    DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.SOSLoggerEnabled = true;
    _LogFilter = PlayerPrefs.GetString("LogFilter");
    SetStatusText("Not Connected");
    RobotEngineManager.Instance.RobotConnected += HandleConnected;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
    RobotEngineManager.Instance.OnFactoryResult += FactoryResult;
    _RestartButton.gameObject.SetActive(false);

    _RestartButton.onClick.AddListener(() => RestartTestApp());
    _LogsButton.onClick.AddListener(HandleLogButtonClick);
    _OptionsButton.onClick.AddListener(HandleOptionsButtonClick);
    _StartButton.onClick.AddListener(HandleStartButtonClick);

    _InProgressSpinner.gameObject.SetActive(false);
  }

  private void HandleSetSimType(bool isSim) {
    _IsSim = isSim;
  }

  private void HandleStartButtonClick() {
    ShowDevConnectDialog();
    _StartButton.gameObject.SetActive(false);
    _InProgressSpinner.gameObject.SetActive(true);
  }

  private void HandleConnected(int robotID) {
    HideDevConnectDialog();
    SetStatusText("Factory App Connected To Robot");

    // runs the factory test.
    RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
    RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.FactoryTest);

    _RestartButton.gameObject.SetActive(true);
  }

  private void HandleLogButtonClick() {
    if (_FactoryLogPanelInstance != null) {
      GameObject.Destroy(_FactoryLogPanelInstance.gameObject);
    }
    if (_FactoryOptionsPanelInstance != null) {
      GameObject.Destroy(_FactoryOptionsPanelInstance.gameObject);
    }
    _FactoryLogPanelInstance = GameObject.Instantiate(_FactoryLogPanelPrefab).GetComponent<FactoryLogPanel>();
    _FactoryLogPanelInstance.transform.SetParent(_Canvas.transform, false);
    _FactoryLogPanelInstance.LogFilter = _LogFilter;
    _FactoryLogPanelInstance.UpdateLogText(_LogList);
  }

  private void HandleOptionsButtonClick() {
    if (_FactoryLogPanelInstance != null) {
      GameObject.Destroy(_FactoryLogPanelInstance.gameObject);
    }
    if (_FactoryOptionsPanelInstance != null) {
      GameObject.Destroy(_FactoryOptionsPanelInstance);
    }
    _FactoryOptionsPanelInstance = GameObject.Instantiate(_FactoryOptionsPanelPrefab).GetComponent<FactoryOptionsPanel>();
    _FactoryOptionsPanelInstance.transform.SetParent(_Canvas.transform, false);
    _FactoryOptionsPanelInstance.OnSetSim += HandleSetSimType;
    _FactoryOptionsPanelInstance.OnOTAButton += HandleOTAButton;
    _FactoryOptionsPanelInstance.OnConsoleLogFilter += HandleSetConsoleLogFilter;
    _FactoryOptionsPanelInstance.Initialize(_IsSim, _LogFilter);
  }

  private void HandleSetConsoleLogFilter(string input) {
    _LogFilter = input;
  }

  private void HandleOTAButton() {
    ShowDevConnectDialog();
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

  private void HandleNewSOSLog(string log_entry) {
    while (_LogList.Count > 9000) {
      _LogList.RemoveAt(0);
    }
    _LogList.Add(log_entry);
    if (_FactoryLogPanelInstance != null) {
      _FactoryLogPanelInstance.UpdateLogText(_LogList);
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
  }

  private void TestPassed() {
    _Background.color = Color.green;
    _InProgressSpinner.gameObject.SetActive(false);
    _RestartButton.gameObject.SetActive(true);
  }

  private void RestartTestApp() {
    _RestartButton.gameObject.SetActive(false);
    _RestartOverlay.gameObject.SetActive(true);
    SOSLogManager.Instance.CleanUp();
    CozmoBinding.Shutdown();
    UnityEngine.SceneManagement.SceneManager.LoadScene("FactoryTest");
  }

  private void FactoryResult(FactoryTestResult result) {
    SetStatusText("Result Code: " + (int)result.resultEntry.result + " (" + result.resultEntry.result + ")");
    if (result.resultEntry.result == Anki.Cozmo.FactoryTestResultCode.SUCCESS) {
      TestPassed();
    }
    else {
      TestFailed();
    }
  }

  public void ShowDevConnectDialog() {
    if (_DevConnectDialogInstance == null && _DevConnectDialog != null) {
      _DevConnectDialogInstance = GameObject.Instantiate(_DevConnectDialog.gameObject);
    }
    _DevConnectDialogInstance.GetComponent<ConnectDialog>().Play(_IsSim);
  }

  private void HideDevConnectDialog() {
    if (_DevConnectDialogInstance != null) {
      Destroy(_DevConnectDialogInstance);
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

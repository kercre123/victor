using UnityEngine;
using System.Collections;
using System.Collections.Generic;

using Anki.Cozmo.ExternalInterface;

public class FactoryIntroManager : MonoBehaviour {

  [SerializeField]
  private Intro _DevConnectDialog;
  private GameObject _DevConnectDialogInstance;

  [SerializeField]
  private UnityEngine.UI.Image _Background;

  [SerializeField]
  private UnityEngine.UI.Button _LogsButton;

  [SerializeField]
  private UnityEngine.UI.Button _OptionsButton;

  [SerializeField]
  private UnityEngine.UI.Button _RestartButton;

  [SerializeField]
  private UnityEngine.UI.Button _StartButton;

  [SerializeField]
  private FactoryLogPanel _FactoryLogPanelPrefab;
  private FactoryLogPanel _FactoryLogPanelInstance;

  [SerializeField]
  private FactoryOptionsPanel _FactoryOptionsPanelPrefab;
  private FactoryOptionsPanel _FactoryOptionsPanelInstance;

  [SerializeField]
  private UnityEngine.UI.Text _StatusText;

  [SerializeField]
  private UnityEngine.UI.Text _StationNumberText;

  [SerializeField]
  private Canvas _Canvas;

  private int _StationNumber = 0;
  private int _TestNumber = 0;

  private List<string> _LogList = new List<string>();

  void Start() {
    
    SetStatusText("Not Connected");
    RobotEngineManager.Instance.RobotConnected += HandleConnected;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
    RobotEngineManager.Instance.OnFactoryResult += FactoryResult;
    _RestartButton.gameObject.SetActive(false);
    _LogsButton.onClick.AddListener(HandleLogButtonClick);
    _OptionsButton.onClick.AddListener(HandleOptionsButtonClick);
    _StartButton.onClick.AddListener(HandleStartButtonClick);
  }

  private void HandleStationNumberUpdate(int update) {
    _StationNumber = update;
    _StationNumberText.text = "Station Number: " + _StationNumber;
    Debug.Log("TODO: Handle Station Number: " + _StationNumber);
  }

  private void HandleTestNumberUpdate(int update) {
    _TestNumber = update;
  }

  private void HandleStartButtonClick() {
    ShowDevConnectDialog();
    _StartButton.gameObject.SetActive(false);
    _RestartButton.gameObject.SetActive(true);
    _RestartButton.onClick.AddListener(() => RestartTestApp());
  }

  private void HandleConnected(int robotID) {
    HideDevConnectDialog();
    SetStatusText("Factory App Connected To Robot");
    RobotEngineManager.Instance.Message.StartFactoryTest = Singleton<StartFactoryTest>.Instance.Initialize((byte)_TestNumber);
    RobotEngineManager.Instance.SendMessage();
    SOSLogManager.Instance.CreateListener();
    RobotEngineManager.Instance.CurrentRobot.SetEnableSOSLogging(true);
    SOSLogManager.Instance.RegisterListener(HandleNewSOSLog);
  }

  private void HandleLogButtonClick() {
    if (_FactoryLogPanelInstance != null) {
      GameObject.Destroy(_FactoryLogPanelInstance.gameObject);
    }
    if (_FactoryOptionsPanelInstance != null) {
      GameObject.Destroy(_FactoryOptionsPanelInstance);
    }
    _FactoryLogPanelInstance = GameObject.Instantiate(_FactoryLogPanelPrefab).GetComponent<FactoryLogPanel>();
    _FactoryLogPanelInstance.transform.SetParent(_Canvas.transform, false);
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
    _FactoryOptionsPanelInstance.OnSetStationNumber += HandleStationNumberUpdate;
    _FactoryOptionsPanelInstance.OnSetTestNumber += HandleTestNumberUpdate;
  }

  private void SetStatusText(string txt) {
    if (_StatusText != null) {
      _StatusText.text = txt;
    }
  }

  private void HandleNewSOSLog(string log_entry) {
    while (_LogList.Count > 100) {
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
  }

  private void TestPassed() {
    _Background.color = Color.green;
  }

  private void RestartTestApp() {
    CozmoBinding.Shutdown();
    UnityEngine.SceneManagement.SceneManager.LoadScene(UnityEngine.SceneManagement.SceneManager.GetActiveScene().name);
  }

  private void FactoryResult(FactoryTestResult result) {
    SetStatusText("Result Code: " + result.resultCode);
    if (result.resultCode != 0) {
      TestFailed();
    }
    else {
      TestPassed();
    }
  }

  private void ShowDevConnectDialog() {
    if (_DevConnectDialogInstance == null && _DevConnectDialog != null) {
      _DevConnectDialogInstance = GameObject.Instantiate(_DevConnectDialog.gameObject);
    }
    _DevConnectDialogInstance.GetComponent<Intro>().Play(false);
  }

  private void HideDevConnectDialog() {
    if (_DevConnectDialogInstance != null) {
      Destroy(_DevConnectDialogInstance);
    }
  }
}

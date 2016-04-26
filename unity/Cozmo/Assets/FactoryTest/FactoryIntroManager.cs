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

  [SerializeField]
  private UnityEngine.UI.Image _InProgressSpinner;

  private int _StationNumber = 0;
  private int _TestNumber = 0;
  private bool _IsSim = false;

  private List<string> _LogList = new List<string>();

  void Start() {
    _RestartButton.onClick.AddListener(() => RestartTestApp());
    SetStatusText("Not Connected");
    RobotEngineManager.Instance.RobotConnected += HandleConnected;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
    RobotEngineManager.Instance.OnFactoryResult += FactoryResult;
    _RestartButton.gameObject.SetActive(false);
    _LogsButton.onClick.AddListener(HandleLogButtonClick);
    _OptionsButton.onClick.AddListener(HandleOptionsButtonClick);
    _StartButton.onClick.AddListener(HandleStartButtonClick);
    _InProgressSpinner.gameObject.SetActive(false);
  }

  private void HandleStationNumberUpdate(int update) {
    _StationNumber = update;
    _StationNumberText.text = "Station Number: " + _StationNumber;
    Debug.Log("TODO: Handle Station Number: " + _StationNumber);
  }

  private void HandleTestNumberUpdate(int update) {
    _TestNumber = update;
    Debug.Log("TODO: Test Start Number: " + _TestNumber);
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

    SOSLogManager.Instance.CreateListener();
    RobotEngineManager.Instance.CurrentRobot.SetEnableSOSLogging(true);
    SOSLogManager.Instance.RegisterListener(HandleNewSOSLog);
    _RestartButton.gameObject.SetActive(true);
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
    _FactoryOptionsPanelInstance.OnSetSim += HandleSetSimType;
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
    SOSLogManager.Instance.CleanUp();
    CozmoBinding.Shutdown();
    UnityEngine.SceneManagement.SceneManager.LoadScene(UnityEngine.SceneManagement.SceneManager.GetActiveScene().name);
  }

  private void FactoryResult(FactoryTestResult result) {
    SetStatusText("Result Code: " + (int)result.resultCode + " (" + result.resultCode + ")");
    if (result.resultCode == Anki.Cozmo.FactoryTestResultCode.SUCCESS) {
      TestPassed();
    }
    else {
      TestFailed();
    }
  }

  private void ShowDevConnectDialog() {
    if (_DevConnectDialogInstance == null && _DevConnectDialog != null) {
      _DevConnectDialogInstance = GameObject.Instantiate(_DevConnectDialog.gameObject);
    }
    _DevConnectDialogInstance.GetComponent<Intro>().Play(_IsSim);
  }

  private void HideDevConnectDialog() {
    if (_DevConnectDialogInstance != null) {
      Destroy(_DevConnectDialogInstance);
    }
  }
}

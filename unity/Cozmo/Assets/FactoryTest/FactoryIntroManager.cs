using UnityEngine;
using System.Collections;

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
  private UnityEngine.UI.Button _SettingsButton;

  [SerializeField]
  private UnityEngine.UI.Button _RestartButton;

  [SerializeField]
  private FactoryLogPanel _FactoryLogPanelPrefab;
  private FactoryLogPanel _FactoryLogPanelInstance;

  [SerializeField]
  private Canvas _Canvas;

  void Start() {
    ShowDevConnectDialog();
    RobotEngineManager.Instance.RobotConnected += HandleConnected;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
    RobotEngineManager.Instance.OnFactoryResult += FactoryResult;
    _RestartButton.gameObject.SetActive(false);
    _LogsButton.onClick.AddListener(HandleLogButtonClick);
  }

  private void HandleConnected(int robotID) {
    HideDevConnectDialog();
    Debug.Log("Factory App Connected To Robot");
    RobotEngineManager.Instance.Message.StartFactoryTest = Singleton<StartFactoryTest>.Instance.Initialize(0);
    RobotEngineManager.Instance.SendMessage();
    SOSLogManager.Instance.CreateListener();
    RobotEngineManager.Instance.CurrentRobot.SetEnableSOSLogging(true);
    SOSLogManager.Instance.RegisterListener(HandleNewSOSLog);
  }

  private void HandleLogButtonClick() {
    if (_FactoryLogPanelInstance != null) {
      GameObject.Destroy(_FactoryLogPanelInstance.gameObject);
    }
    _FactoryLogPanelInstance = GameObject.Instantiate(_FactoryLogPanelPrefab).GetComponent<FactoryLogPanel>();
    _FactoryLogPanelInstance.transform.SetParent(_Canvas.transform, false);
  }

  private void HandleNewSOSLog(string log_entry) {
    
  }

  private void HandleDisconnectedFromClient(DisconnectionReason obj) {
    TestFailed();
  }

  private void TestFailed() {
    if (_Background == null) {
      return; // quit case
    }
    _Background.color = Color.red;
    _RestartButton.gameObject.SetActive(true);
  }

  private void TestPassed() {
    _Background.color = Color.green;
    _RestartButton.gameObject.SetActive(true);
  }

  private void FactoryResult(FactoryTestResult result) {
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

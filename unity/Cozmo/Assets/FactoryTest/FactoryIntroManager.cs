using UnityEngine;
using System.Collections;

using Anki.Cozmo.ExternalInterface;

public class FactoryIntroManager : MonoBehaviour {

  [SerializeField]
  private Intro _DevConnectDialog;
  private GameObject _DevConnectDialogInstance;

  void Start() {
    ShowDevConnectDialog();
    RobotEngineManager.Instance.RobotConnected += HandleConnected;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
    RobotEngineManager.Instance.OnFactoryResult += FactoryResult;
  }

  private void HandleConnected(int robotID) {
    HideDevConnectDialog();
    Debug.Log("Factory App Connected To Robot");
    RobotEngineManager.Instance.Message.StartFactoryTest = Singleton<StartFactoryTest>.Instance.Initialize(0);
    RobotEngineManager.Instance.SendMessage();
  }

  private void HandleDisconnectedFromClient(DisconnectionReason obj) {
    TestFailed();
  }

  private void TestFailed() {
    
  }

  private void FactoryResult(FactoryTestResult result) {
    if (result.resultCode > 0) {
      TestFailed();
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

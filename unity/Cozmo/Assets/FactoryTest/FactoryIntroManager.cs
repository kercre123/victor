using UnityEngine;
using System.Collections;

public class FactoryIntroManager : MonoBehaviour {

  [SerializeField]
  private Intro _DevConnectDialog;
  private GameObject _DevConnectDialogInstance;

  void Start() {
    ShowDevConnectDialog();
    RobotEngineManager.Instance.RobotConnected += HandleConnected;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
  }

  private void HandleConnected(int robotID) {
    HideDevConnectDialog();
    Debug.Log("Factory App Connected");
  }

  private void HandleDisconnectedFromClient(DisconnectionReason obj) {
    ShowDevConnectDialog();
  }

  private void ShowDevConnectDialog() {
    if (_DevConnectDialogInstance == null && _DevConnectDialog != null) {
      _DevConnectDialogInstance = UIManager.CreateUIElement(_DevConnectDialog.gameObject);
    }
    _DevConnectDialogInstance.GetComponent<Intro>().Play(false);
  }

  private void HideDevConnectDialog() {
    if (_DevConnectDialogInstance != null) {
      Destroy(_DevConnectDialogInstance);
    }
  }
}

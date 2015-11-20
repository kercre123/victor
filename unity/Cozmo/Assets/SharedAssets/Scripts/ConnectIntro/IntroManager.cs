using UnityEngine;
using System.Collections;

public class IntroManager : MonoBehaviour {

  [SerializeField]
  private Intro _DevConnectDialog;
  private GameObject _DevConnectDialogInstance;

  [SerializeField]
  private HubWorldBase _HubWorldPrefab;
  private HubWorldBase _HubWorldInstance;

  void Start() {
    ShowDevConnectDialog();

    RobotEngineManager.Instance.RobotConnected += Connected;
    RobotEngineManager.Instance.DisconnectedFromClient += DisconnectedFromClient;
  }

  void Connected(int robotID) {
    // Spawn HubWorld
    GameObject hubWorldObject = GameObject.Instantiate(_HubWorldPrefab.gameObject);
    hubWorldObject.transform.SetParent(transform, false);
    _HubWorldInstance = hubWorldObject.GetComponent<HubWorldBase>();

    // Hide Intro dialog when it finishes loading
    if (_HubWorldInstance.LoadHubWorld()) {
      HideDevConnectDialog();
    }
  }

  void DisconnectedFromClient(DisconnectionReason obj) {
    // Force quit hub world and show connect dialog again
    if (_HubWorldInstance != null) {
      _HubWorldInstance.DestroyHubWorld(); 
      Destroy(_HubWorldInstance);
    }

    UIManager.CloseAllViewsImmediately();
    ShowDevConnectDialog();
  }

  private void ShowDevConnectDialog() {
    if (_DevConnectDialogInstance == null) {
      _DevConnectDialogInstance = UIManager.CreateUIElement(_DevConnectDialog.gameObject);
    }
  }

  private void HideDevConnectDialog() {
    if (_DevConnectDialogInstance != null) {
      Destroy(_DevConnectDialogInstance);
    }
  }
}

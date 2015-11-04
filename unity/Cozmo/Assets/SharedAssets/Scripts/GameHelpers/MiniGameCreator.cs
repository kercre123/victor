using UnityEngine;
using System.Collections;

public class MiniGameCreator : MonoBehaviour {

  [SerializeField]
  GameBase gamePrefab_;

  [SerializeField]
  private Intro devConnectDialog_;
  private GameObject devConnectDialogInstance_;

  void Start() {
    ShowDevConnectDialog();
    RobotEngineManager.instance.RobotConnected += Connected;
    RobotEngineManager.instance.DisconnectedFromClient += DisconnectedFromClient;
  }

  private void OnDestroy(){
    if (devConnectDialogInstance_ != null) {
      Destroy(devConnectDialogInstance_);
    }
  }

  void Connected(int robotID) {
    HideDevConnectDialog();
    GameObject.Instantiate(gamePrefab_);
  }
  
  void DisconnectedFromClient (DisconnectionReason obj)
  {
    ShowDevConnectDialog();
  }

  private void ShowDevConnectDialog(){
    if (devConnectDialogInstance_ == null) {
      devConnectDialogInstance_ = UIManager.CreateUI(devConnectDialog_.gameObject);
    }
  }

  private void HideDevConnectDialog(){
    if (devConnectDialogInstance_ != null) {
      Destroy(devConnectDialogInstance_);
    }
  }
}

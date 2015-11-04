using UnityEngine;
using System.Collections;

public class IntroManager : MonoBehaviour {

  [SerializeField]
  private Intro devConnectDialog_;
  private GameObject devConnectDialogInstance_;

  [SerializeField]
  private HubWorldBase hubWorldPrefab_;
  private HubWorldBase hubWorldInstance_;
  
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
    // Spawn HubWorld
    GameObject hubWorldObject = GameObject.Instantiate(hubWorldPrefab_.gameObject);
    hubWorldInstance_ = hubWorldObject.GetComponent<HubWorldBase>();

    // Hide Intro dialog when it finishes loading
    if (hubWorldInstance_.LoadHubWorld()) {
      HideDevConnectDialog();
    }
  }
  
  void DisconnectedFromClient (DisconnectionReason obj)
  {
    // Force quit hub world and show connect dialog again
    hubWorldInstance_.DestroyHubWorld();
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

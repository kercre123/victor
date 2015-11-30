using UnityEngine;
using System.Collections;

public class IntroManager : MonoBehaviour {

  [SerializeField]
  private Intro _DevConnectDialog;
  private GameObject _DevConnectDialogInstance;

  [SerializeField]
  private HubWorldBase _HubWorldPrefab;
  private HubWorldBase _HubWorldInstance;

  [SerializeField]
  private string _IntroScriptedSequenceName;

  private ScriptedSequences.ISimpleAsyncToken _IntroSequenceDoneToken;

  void Start() {
    ShowDevConnectDialog();

    RobotEngineManager.Instance.RobotConnected += HandleConnected;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
  }

  private void HandleConnected(int robotID) {

    HideDevConnectDialog();

    if (_IntroScriptedSequenceName.Equals("")) {
      HandleIntroSequenceDone(_IntroSequenceDoneToken);
    }
    else {
      _IntroSequenceDoneToken = ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence("IntroSequence");
      _IntroSequenceDoneToken.Ready(HandleIntroSequenceDone);
    }
  }

  private void HandleDisconnectedFromClient(DisconnectionReason obj) {
    // Force quit hub world and show connect dialog again
    if (_HubWorldInstance != null) {
      _HubWorldInstance.DestroyHubWorld(); 
      Destroy(_HubWorldInstance);
    }

    UIManager.CloseAllViewsImmediately();
    ShowDevConnectDialog();
  }

  private void HandleIntroSequenceDone(ScriptedSequences.ISimpleAsyncToken token) {
    // Spawn HubWorld
    GameObject hubWorldObject = GameObject.Instantiate(_HubWorldPrefab.gameObject);
    hubWorldObject.transform.SetParent(transform, false);
    _HubWorldInstance = hubWorldObject.GetComponent<HubWorldBase>();

    _HubWorldInstance.LoadHubWorld();
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

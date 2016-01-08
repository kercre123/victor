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

  [SerializeField]
  private RobotStateTextField _RobotStateTextFieldPrefab;
  private RobotStateTextField _RobotStateTextFieldInstance;

  private ScriptedSequences.ISimpleAsyncToken _IntroSequenceDoneToken;

  void Start() {
    ShowDevConnectDialog();

    RobotEngineManager.Instance.RobotConnected += HandleConnected;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
  }

  private void HandleConnected(int robotID) {

    HideDevConnectDialog();

    if (string.IsNullOrEmpty(_IntroScriptedSequenceName)) {
      HandleIntroSequenceDone(_IntroSequenceDoneToken);
    }
    else {
      _IntroSequenceDoneToken = ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence(_IntroScriptedSequenceName);
      _IntroSequenceDoneToken.Ready(HandleIntroSequenceDone);
    }

    if (_RobotStateTextFieldInstance == null && _RobotStateTextFieldPrefab != null) {
      _RobotStateTextFieldInstance = UIManager.CreateUIElement(_RobotStateTextFieldPrefab).GetComponent<RobotStateTextField>();
    }
  }

  private void HandleDisconnectedFromClient(DisconnectionReason obj) {
    // Force quit hub world and show connect dialog again
    if (_HubWorldInstance != null) {
      _HubWorldInstance.DestroyHubWorld(); 
      Destroy(_HubWorldInstance);
    }

    UIManager.CloseAllViewsImmediately();
    if (_RobotStateTextFieldInstance != null) {
      Destroy(_RobotStateTextFieldInstance.gameObject);
    }

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

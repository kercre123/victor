using UnityEngine;
using System.Collections;

public class IntroManager : MonoBehaviour {

  [SerializeField]
  private ConnectDialog _ConnectDialogPrefab;
  private GameObject _ConnectDialogInstance;

  [SerializeField]
  private HubWorldBase _HubWorldPrefab;
  private HubWorldBase _HubWorldInstance;

  [SerializeField]
  private string _IntroScriptedSequenceName;

  private ScriptedSequences.ISimpleAsyncToken _IntroSequenceDoneToken;

  void Start() {
    ShowDevConnectDialog();

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(HandleConnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnectedFromClient);
  }

  void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(HandleConnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnectedFromClient);
  }

  private void HandleConnected(object message) {

    HideDevConnectDialog();

    if (string.IsNullOrEmpty(_IntroScriptedSequenceName)) {
      HandleIntroSequenceDone(_IntroSequenceDoneToken);
    }
    else {
      _IntroSequenceDoneToken = ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence(_IntroScriptedSequenceName);
      _IntroSequenceDoneToken.Ready(HandleIntroSequenceDone);
    }
  }

  private void HandleDisconnectedFromClient(object message) {
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
    if (_ConnectDialogInstance == null && _ConnectDialogPrefab != null) {
      _ConnectDialogInstance = UIManager.CreateUIElement(_ConnectDialogPrefab.gameObject);
    }
  }

  private void HideDevConnectDialog() {
    if (_ConnectDialogInstance != null) {
      Destroy(_ConnectDialogInstance);
    }
  }
}

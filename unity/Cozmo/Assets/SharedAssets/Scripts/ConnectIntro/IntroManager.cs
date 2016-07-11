using UnityEngine;
using Anki.Assets;

public class IntroManager : MonoBehaviour {

  [SerializeField]
  private GameObjectDataLink _ConnectDialogPrefabData;

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
    AssetBundleManager.Instance.LoadAssetBundleAsync(_ConnectDialogPrefabData.AssetBundle, LoadConnectView);
  }

  private void LoadConnectView(bool assetBundleSuccess) {
    _ConnectDialogPrefabData.LoadAssetData((GameObject connectViewPrefab) => {
      if (_ConnectDialogInstance == null && connectViewPrefab != null) {
        _ConnectDialogInstance = UIManager.CreateUIElement(connectViewPrefab.gameObject);
      }
    });
  }

  private void HideDevConnectDialog() {
    if (_ConnectDialogInstance != null) {
      Destroy(_ConnectDialogInstance);
      _ConnectDialogInstance = null;

      // INGO
      // Right now StartView and ConnectDialog use the same assets so don't bother unloading asset bundle.
      // AssetBundleManager.Instance.UnloadAssetBundle(_ConnectDialogPrefabData.AssetBundle);
    }

    // Don't unload the asset bundle because other assets use it
  }
}

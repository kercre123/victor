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
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(HandleRobotConnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnected);
    ShowConnectDialog();
#if !UNITY_EDITOR
    SetupEngine();
#endif
  }

  void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(HandleRobotConnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnected);
  }

  private void SetupEngine() {
    RobotEngineManager.Instance.StartEngine();
    // Set initial volumes
    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues();
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
  }

  private void HandleRobotConnected(Anki.Cozmo.ExternalInterface.RobotConnected message) {

    HideConnectDialog();

    if (string.IsNullOrEmpty(_IntroScriptedSequenceName)) {
      HandleIntroSequenceDone(_IntroSequenceDoneToken);
    }
    else {
      _IntroSequenceDoneToken = ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence(_IntroScriptedSequenceName);
      _IntroSequenceDoneToken.Ready(HandleIntroSequenceDone);
    }
  }

  private void HandleDisconnected(Anki.Cozmo.ExternalInterface.RobotDisconnected message) {
    // Force quit hub world and show connect dialog again
    if (_HubWorldInstance != null) {
      _HubWorldInstance.DestroyHubWorld();
      Destroy(_HubWorldInstance);
    }

    UIManager.CloseAllViewsImmediately();

    ShowConnectDialog();
  }

  private void HandleIntroSequenceDone(ScriptedSequences.ISimpleAsyncToken token) {
    // Spawn HubWorld
    GameObject hubWorldObject = GameObject.Instantiate(_HubWorldPrefab.gameObject);
    hubWorldObject.transform.SetParent(transform, false);
    _HubWorldInstance = hubWorldObject.GetComponent<HubWorldBase>();

    _HubWorldInstance.LoadHubWorld();
  }

  private void ShowConnectDialog() {
    AssetBundleManager.Instance.LoadAssetBundleAsync(_ConnectDialogPrefabData.AssetBundle, LoadConnectView);
  }

  private void LoadConnectView(bool assetBundleSuccess) {
    _ConnectDialogPrefabData.LoadAssetData((GameObject connectViewPrefab) => {
      if (_ConnectDialogInstance == null && connectViewPrefab != null) {
        _ConnectDialogInstance = UIManager.CreateUIElement(connectViewPrefab.gameObject);
      }
    });
  }

  private void HideConnectDialog() {
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

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
    ShowConnectDialog();
#if !UNITY_EDITOR
    SetupEngine();
#endif
  }

  private void SetupEngine() {
    RobotEngineManager.Instance.StartEngine();
    // Set initial volumes
    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues();
  }

  private void HandleRobotConnected() {

    HideConnectDialog();

    if (string.IsNullOrEmpty(_IntroScriptedSequenceName)) {
      HandleIntroSequenceDone(_IntroSequenceDoneToken);
    }
    else {
      _IntroSequenceDoneToken = ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence(_IntroScriptedSequenceName);
      _IntroSequenceDoneToken.Ready(HandleIntroSequenceDone);
    }
  }

  private void HandleIntroSequenceDone(ScriptedSequences.ISimpleAsyncToken token) {
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
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
        _ConnectDialogInstance.GetComponent<ConnectDialog>().ConnectionFlowComplete += HandleRobotConnected;
      }
    });

  }

  private void HideConnectDialog() {
    if (_ConnectDialogInstance != null) {
      _ConnectDialogInstance.GetComponent<ConnectDialog>().ConnectionFlowComplete -= HandleRobotConnected;
      GameObject.Destroy(_ConnectDialogInstance);
      _ConnectDialogInstance = null;

      // INGO
      // Right now StartView and ConnectDialog use the same assets so don't bother unloading asset bundle.
      // AssetBundleManager.Instance.UnloadAssetBundle(_ConnectDialogPrefabData.AssetBundle);
    }

    // Don't unload the asset bundle because other assets use it
  }
}

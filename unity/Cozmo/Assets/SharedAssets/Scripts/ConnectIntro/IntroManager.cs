using UnityEngine;
using Anki.Assets;

public class IntroManager : MonoBehaviour {

  [SerializeField]
  private GameObjectDataLink _SimpleConnectDialogPrefabData;

  private GameObject _SimpleConnectDialogInstance;

  [SerializeField]
  private HubWorldBase _HubWorldPrefab;
  private HubWorldBase _HubWorldInstance;

  [SerializeField]
  private string _IntroScriptedSequenceName;

  private ScriptedSequences.ISimpleAsyncToken _IntroSequenceDoneToken;

  void Start() {

    Application.targetFrameRate = 30;
    Screen.sleepTimeout = SleepTimeout.NeverSleep;
    Input.gyro.enabled = true;
    Input.compass.enabled = true;
    Input.multiTouchEnabled = true;

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

  private void HandleConnectionFlowComplete() {

    HideConnectDialog();

    if (string.IsNullOrEmpty(_IntroScriptedSequenceName)) {
      HandleIntroSequenceDone(_IntroSequenceDoneToken);
    }
    else {
      _IntroSequenceDoneToken = ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence(_IntroScriptedSequenceName);
      _IntroSequenceDoneToken.Ready(HandleIntroSequenceDone);
    }
  }

  private void HandleConnectionFlowQuit() {
    // destroy and re-create the connect dialog to restart the flow
    DAS.Info("IntroManager.HandleConnectionFlowQuit", "Restarting Connection Dialog Flow");
    HideConnectDialog();
    ShowConnectDialog();
  }

  private void HandleIntroSequenceDone(ScriptedSequences.ISimpleAsyncToken token) {
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);

    GameObject hubWorldObject = GameObject.Instantiate(_HubWorldPrefab.gameObject);
    hubWorldObject.transform.SetParent(transform, false);
    _HubWorldInstance = hubWorldObject.GetComponent<HubWorldBase>();
    _HubWorldInstance.LoadHubWorld();
  }

  private void ShowConnectDialog() {
    AssetBundleManager.Instance.LoadAssetBundleAsync(_SimpleConnectDialogPrefabData.AssetBundle, LoadConnectView);
  }

  private void LoadConnectView(bool assetBundleSuccess) {
    _SimpleConnectDialogPrefabData.LoadAssetData((GameObject connectViewPrefab) => {
      if (_SimpleConnectDialogInstance == null && connectViewPrefab != null) {
        _SimpleConnectDialogInstance = UIManager.CreateUIElement(connectViewPrefab.gameObject);
        _SimpleConnectDialogInstance.GetComponent<SimpleConnectDialog>().ConnectionFlowComplete += HandleConnectionFlowComplete;
        _SimpleConnectDialogInstance.GetComponent<SimpleConnectDialog>().ConnectionFlowQuit += HandleConnectionFlowQuit;
      }
    });
  }

  private void HideConnectDialog() {
    AssetBundleManager.Instance.UnloadAssetBundle(_SimpleConnectDialogPrefabData.AssetBundle);
    if (_SimpleConnectDialogInstance != null) {
      _SimpleConnectDialogInstance.GetComponent<SimpleConnectDialog>().ConnectionFlowComplete -= HandleConnectionFlowComplete;
      _SimpleConnectDialogInstance.GetComponent<SimpleConnectDialog>().ConnectionFlowQuit -= HandleConnectionFlowQuit;
      GameObject.Destroy(_SimpleConnectDialogInstance);
      _SimpleConnectDialogInstance = null;
    }
  }
}

using UnityEngine;
using Anki.Assets;
using System;

public class IntroManager : MonoBehaviour {

  private static IntroManager _Instance;

  public static IntroManager Instance {
    get {
      return _Instance;
    }
  }

  [SerializeField]
  private GameObjectDataLink _FirstTimeConnectDialogPrefabData;
  private GameObject _FirstTimeConnectDialogInstance;

  [SerializeField]
  private GameObjectDataLink _CheckInDialogPrefabData;
  private GameObject _CheckInDialogInstance;

  [SerializeField]
  private HubWorldBase _HubWorldPrefab;
  private HubWorldBase _HubWorldInstance;

  private ScriptedSequences.ISimpleAsyncToken _IntroSequenceDoneToken;

  void Start() {
    if (_Instance != null) {
      Destroy(this);
    }
    else {
      _Instance = this;
    }
    Application.targetFrameRate = 30;

    Screen.sleepTimeout = SleepTimeout.NeverSleep;
    Input.gyro.enabled = true;
    Input.compass.enabled = true;
    Input.multiTouchEnabled = true;

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(OnRobotDisconnect);

    StartFlow();
    SetupEngine();

    // If SDK Only, force flag to true, this build flag is also used to hide any options to disable SDK mode
#if SDK_ONLY
    DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.IsSDKEnabled = true;
    DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.SDKActivated = true;
    DataPersistence.DataPersistenceManager.Instance.Save();
#endif

  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(OnRobotDisconnect);
  }

  private void StartFlow() {
    if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Home)) {
      OnboardingManager.Instance.PreloadOnboarding();
      ShowFirstTimeFlow();
    }
    else {
      ShowCheckInFlow();
    }
  }

  private void SetupEngine() {
    RobotEngineManager.Instance.StartEngine();
    // Set initial volumes
    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues();
  }

  public void ForceBoot() {
    RobotEngineManager.Instance.StartIdleTimeout(0f, 0f);
    OnRobotDisconnect(new Anki.Cozmo.ExternalInterface.RobotDisconnected());
  }

  private void OnRobotDisconnect(object message) {
    if (null != _HubWorldInstance) {
      _HubWorldInstance.DestroyHubWorld();
    }

    // if we are in a connection flow then they should handle the disconnects properly.
    if (_FirstTimeConnectDialogInstance != null) {
      _FirstTimeConnectDialogInstance.GetComponent<FirstTimeConnectDialog>().HandleRobotDisconnect();
      return;
    }
    else if (_CheckInDialogInstance != null) {
      _CheckInDialogInstance.GetComponent<CheckInFlow>().HandleRobotDisconnect();
      return;
    }
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
    UIManager.CloseAllViewsImmediately();
    UIManager.EnableTouchEvents();
    StartFlow();
  }

  private void HandleFirstTimeConnectionFlowComplete() {
    HideFirstTimeConnectDialog();
    IntroFlowComplete();
  }

  private void HandleCheckinFlowComplete() {
    HideCheckInFlow();
    IntroFlowComplete();
  }

  private void HandleFirstTimeConnectFlowQuit() {
    // destroy and re-create the connect dialog to restart the flow
    DAS.Info("IntroManager.HandleConnectionFlowQuit", "Restarting Connection Dialog Flow");
    HideFirstTimeConnectDialog();
    ShowFirstTimeConnectDialog();
  }

  private void ShowFirstTimeConnectDialog() {
    AssetBundleManager.Instance.LoadAssetBundleAsync(_FirstTimeConnectDialogPrefabData.AssetBundle, LoadConnectView);
  }

  private void HandleCheckInFlowQuit() {
    // destroy and re-create the connect dialog to restart the flow
    DAS.Info("IntroManager.HandleCheckInFlowQuit", "Restarting CheckIn Dialog Flow");
    HideCheckInFlow();
    ShowCheckInFlow();
  }

  private void ShowFirstTimeFlow() {
    AssetBundleManager.Instance.LoadAssetBundleAsync(_FirstTimeConnectDialogPrefabData.AssetBundle, LoadConnectView);
  }

  private void LoadConnectView(bool assetBundleSuccess) {
    _FirstTimeConnectDialogPrefabData.LoadAssetData((GameObject connectViewPrefab) => {
      if (_FirstTimeConnectDialogInstance == null && connectViewPrefab != null) {
        _FirstTimeConnectDialogInstance = UIManager.CreateUIElement(connectViewPrefab.gameObject);
        _FirstTimeConnectDialogInstance.GetComponent<FirstTimeConnectDialog>().ConnectionFlowComplete += HandleFirstTimeConnectionFlowComplete;
        _FirstTimeConnectDialogInstance.GetComponent<FirstTimeConnectDialog>().ConnectionFlowQuit += HandleFirstTimeConnectFlowQuit;
      }
    });
  }

  private void HideFirstTimeConnectDialog() {
    AssetBundleManager.Instance.UnloadAssetBundle(_FirstTimeConnectDialogPrefabData.AssetBundle);
    if (_FirstTimeConnectDialogInstance != null) {
      _FirstTimeConnectDialogInstance.GetComponent<FirstTimeConnectDialog>().ConnectionFlowComplete -= HandleFirstTimeConnectionFlowComplete;
      _FirstTimeConnectDialogInstance.GetComponent<FirstTimeConnectDialog>().ConnectionFlowQuit -= HandleFirstTimeConnectFlowQuit;
      GameObject.Destroy(_FirstTimeConnectDialogInstance);
      _FirstTimeConnectDialogInstance = null;
    }
  }

  private void IntroFlowComplete() {
    GameObject hubWorldObject = GameObject.Instantiate(_HubWorldPrefab.gameObject);
    hubWorldObject.transform.SetParent(transform, false);
    _HubWorldInstance = hubWorldObject.GetComponent<HubWorldBase>();
    _HubWorldInstance.LoadHubWorld();
  }

  private void ShowCheckInFlow() {
    AssetBundleManager.Instance.LoadAssetBundleAsync(_CheckInDialogPrefabData.AssetBundle, LoadCheckInView);
  }

  private void LoadCheckInView(bool assetBundleSuccess) {
    _CheckInDialogPrefabData.LoadAssetData((GameObject checkInViewPrefab) => {
      if (_CheckInDialogInstance == null && checkInViewPrefab != null) {
        _CheckInDialogInstance = UIManager.CreateUIElement(checkInViewPrefab.gameObject);
        _CheckInDialogInstance.GetComponent<CheckInFlow>().ConnectionFlowComplete += HandleCheckinFlowComplete;
        _CheckInDialogInstance.GetComponent<CheckInFlow>().CheckInFlowQuit += HandleCheckInFlowQuit;
      }
    });
  }

  private void HideCheckInFlow() {
    AssetBundleManager.Instance.UnloadAssetBundle(_CheckInDialogPrefabData.AssetBundle);
    if (_CheckInDialogInstance != null) {
      _CheckInDialogInstance.GetComponent<CheckInFlow>().ConnectionFlowComplete -= HandleCheckinFlowComplete;
      _CheckInDialogInstance.GetComponent<CheckInFlow>().CheckInFlowQuit -= HandleCheckInFlowQuit;
      GameObject.Destroy(_CheckInDialogInstance);
      _CheckInDialogInstance = null;
    }
  }

}

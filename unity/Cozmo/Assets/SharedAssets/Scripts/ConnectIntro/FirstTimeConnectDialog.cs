using UnityEngine;
using UnityEngine.UI;

public class FirstTimeConnectDialog : MonoBehaviour {

  public System.Action ConnectionFlowComplete;
  public System.Action ConnectionFlowQuit;

  [SerializeField]
  private Cozmo.UI.CozmoButton _StartButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SimButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _MockButton;

  [SerializeField]
  private ConnectionFlow _ConnectionFlowPrefab;
  private ConnectionFlow _ConnectionFlowInstance;

  [SerializeField]
  private SoundCheckView _SoundCheckViewPrefab;
  private SoundCheckView _SoundCheckViewInstance;

  [SerializeField]
  private SimpleConnectView _PlaceCozmoOnChargerConnectViewPrefab;
  private SimpleConnectView _PlaceCozmoOnChargerConnectViewInstance;

  [SerializeField]
  private ProfileCreationView _ProfileCreationViewPrefab;
  private ProfileCreationView _ProfileCreationViewInstance;

  private void Start() {

    _StartButton.Initialize(HandleStartButton, "start_button", "simple_connect_dialog");
    _SimButton.Initialize(HandleSimButton, "sim_button", "simple_connect_dialog");
    _MockButton.Initialize(HandleMockButton, "mock_button", "simple_connect_dialog");

#if !UNITY_EDITOR
    // hide sim and mock buttons for on device deployments
    _SimButton.gameObject.SetActive(false);
    _MockButton.gameObject.SetActive(false);
#endif

    _StartButton.Text = Localization.Get(LocalizationKeys.kLabelStart);
    UIManager.Instance.BackgroundColorController.SetBackgroundColor(Cozmo.UI.BackgroundColorController.BackgroundColor.Yellow);
  }

  private void OnDestroy() {
    if (_PlaceCozmoOnChargerConnectViewInstance != null) {
      UIManager.CloseViewImmediately(_PlaceCozmoOnChargerConnectViewInstance);
    }

    if (_ConnectionFlowInstance != null) {
      GameObject.Destroy(_ConnectionFlowInstance.gameObject);
    }
  }

  private void HandleStartButton() {
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileCreated) {
      ShowPlaceCozmoOnCharger();
    }
    else {
      _SoundCheckViewInstance = UIManager.OpenView(_SoundCheckViewPrefab);
      _SoundCheckViewInstance.OnSoundCheckComplete += HandleSoundCheckComplete;
    }
  }

  private void HandleSoundCheckComplete() {
    UIManager.CloseView(_SoundCheckViewInstance);
    ShowProfileCreationScreen();
  }

  private void ShowProfileCreationScreen() {
    _ProfileCreationViewInstance = UIManager.OpenView(_ProfileCreationViewPrefab);
    _ProfileCreationViewInstance.ViewClosed += HandleProfileCreationDone;
  }

  private void HandleProfileCreationDone() {
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileCreated = true;
    ShowPlaceCozmoOnCharger();
  }

  private void ShowPlaceCozmoOnCharger() {
    _PlaceCozmoOnChargerConnectViewInstance = UIManager.OpenView(_PlaceCozmoOnChargerConnectViewPrefab);
    _PlaceCozmoOnChargerConnectViewInstance.OnConnectButton += HandleConnectButton;
  }

  private void HandleMockButton() {
    RobotEngineManager.Instance.MockConnect();
    if (ConnectionFlowComplete != null) {
      ConnectionFlowComplete();
    }
  }

  private void HandleConnectButton() {
    _PlaceCozmoOnChargerConnectViewInstance.ViewClosed += StartConnectionFlow;
    UIManager.CloseView(_PlaceCozmoOnChargerConnectViewInstance);
  }

  private void StartConnectionFlow() {
    _ConnectionFlowInstance = GameObject.Instantiate(_ConnectionFlowPrefab.gameObject).GetComponent<ConnectionFlow>();
    _ConnectionFlowInstance.ConnectionFlowComplete += HandleConnectionFlowComplete;
    _ConnectionFlowInstance.ConnectionFlowQuit += HandleConnectionFlowQuit;
    _ConnectionFlowInstance.Play(sim: false);
  }

  private void HandleSimButton() {
    _ConnectionFlowInstance = GameObject.Instantiate(_ConnectionFlowPrefab.gameObject).GetComponent<ConnectionFlow>();
    _ConnectionFlowInstance.ConnectionFlowComplete += HandleConnectionFlowComplete;
    _ConnectionFlowInstance.ConnectionFlowQuit += HandleConnectionFlowQuit;
    _ConnectionFlowInstance.Play(sim: true);
  }

  private void HandleConnectionFlowComplete() {
    // generate the initial set of daily goals
    DataPersistence.DataPersistenceManager.Instance.StartNewSession();

    if (_ConnectionFlowInstance != null) {
      GameObject.Destroy(_ConnectionFlowInstance.gameObject);
    }
    if (ConnectionFlowComplete != null) {
      ConnectionFlowComplete();
    }

  }

  private void HandleConnectionFlowQuit() {
    if (_ConnectionFlowInstance != null) {
      GameObject.Destroy(_ConnectionFlowInstance.gameObject);
    }
    if (ConnectionFlowQuit != null) {
      ConnectionFlowQuit();
    }
  }
}

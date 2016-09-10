using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;

public class FirstTimeConnectDialog : MonoBehaviour {

  public System.Action ConnectionFlowComplete;
  public System.Action ConnectionFlowQuit;

  [SerializeField]
  private Cozmo.UI.CozmoButton _StartButton;

  [SerializeField]
  private ConnectionFlow _ConnectionFlowPrefab;
  private ConnectionFlow _ConnectionFlowInstance;

  [SerializeField]
  private SoundCheckView _SoundCheckViewPrefab;
  private SoundCheckView _SoundCheckViewInstance;

  [SerializeField]
  private PlaceCozmoOnChargerView _PlaceCozmoOnChargerViewPrefab;
  private PlaceCozmoOnChargerView _PlaceCozmoOnChargerViewInstance;

  [SerializeField]
  private ProfileCreationView _ProfileCreationViewPrefab;
  private ProfileCreationView _ProfileCreationViewInstance;

  [SerializeField]
  private Cozmo.UI.ScrollingTextView _PrivacyPolicyViewPrefab;

  [SerializeField]
  private Cozmo.UI.CozmoButton _PrivacyPolicyButton;

  private void Awake() {

    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
      _StartButton.Initialize(HandleMockButton, "start_button", "first_time_connect_dialog");
    }
    else {
      _StartButton.Initialize(HandleStartButton, "start_button", "first_time_connect_dialog");
    }

    _PrivacyPolicyButton.Initialize(() => {
      ScrollingTextView view = UIManager.OpenView<ScrollingTextView>(_PrivacyPolicyViewPrefab, (ScrollingTextView v) => { v.DASEventViewName = "privacy_policy_view"; });
      view.Initialize(LocalizationKeys.kPrivacyPolicyTitle, LocalizationKeys.kPrivacyPolicyText);
    }, "privacy_policy_button", "first_time_connect_dialog");

    _StartButton.Text = Localization.Get(LocalizationKeys.kLabelStart);
    UIManager.Instance.BackgroundColorController.SetBackgroundColor(Cozmo.UI.BackgroundColorController.BackgroundColor.Yellow);
  }

  private void OnDestroy() {
    if (_PlaceCozmoOnChargerViewInstance != null) {
      UIManager.CloseViewImmediately(_PlaceCozmoOnChargerViewInstance);
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
    _SoundCheckViewInstance.ViewClosed += ShowProfileCreationScreen;
    UIManager.CloseView(_SoundCheckViewInstance);
  }

  private void ShowProfileCreationScreen() {
    _ProfileCreationViewInstance = UIManager.OpenView(_ProfileCreationViewPrefab);
    _ProfileCreationViewInstance.ViewClosed += HandleProfileCreationDone;
  }

  private void HandleProfileCreationDone() {
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileCreated = true;
    DataPersistence.DataPersistenceManager.Instance.Save();
    ShowPlaceCozmoOnCharger();
  }

  private void ShowPlaceCozmoOnCharger() {
    _PlaceCozmoOnChargerViewInstance = UIManager.OpenView(_PlaceCozmoOnChargerViewPrefab);
    _PlaceCozmoOnChargerViewInstance.OnConnectButton += HandleConnectButton;
  }

  private void HandleMockButton() {
    RobotEngineManager.Instance.MockConnect();
    if (DataPersistence.DataPersistenceManager.Instance.IsNewSessionNeeded) {
      DataPersistence.DataPersistenceManager.Instance.StartNewSession();
    }
    if (ConnectionFlowComplete != null) {
      ConnectionFlowComplete();
    }
  }

  private void HandleConnectButton() {
    _PlaceCozmoOnChargerViewInstance.ViewClosed += StartConnectionFlow;
    UIManager.CloseView(_PlaceCozmoOnChargerViewInstance);
  }

  private void StartConnectionFlow() {
    _ConnectionFlowInstance = GameObject.Instantiate(_ConnectionFlowPrefab.gameObject).GetComponent<ConnectionFlow>();
    _ConnectionFlowInstance.ConnectionFlowComplete += HandleConnectionFlowComplete;
    _ConnectionFlowInstance.ConnectionFlowQuit += HandleConnectionFlowQuit;
    _ConnectionFlowInstance.StartConnectionFlow();
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

  public void HandleRobotDisconnect() {
    if (_ConnectionFlowInstance != null) {
      _ConnectionFlowInstance.HandleRobotDisconnect();
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

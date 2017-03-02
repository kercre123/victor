using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;

public class FirstTimeConnectView : BaseView {

  public System.Action ConnectionFlowComplete;
  public System.Action ConnectionFlowQuit;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _StartButton;

  [SerializeField]
  private ConnectionFlowController _ConnectionFlowPrefab;
  private ConnectionFlowController _ConnectionFlowInstance;

  [SerializeField]
  private SoundCheckModal _SoundCheckModalPrefab;
  private SoundCheckModal _SoundCheckModalInstance;

  [SerializeField]
  private PlaceCozmoOnChargerModal _PlaceCozmoOnChargerModalPrefab;
  private PlaceCozmoOnChargerModal _PlaceCozmoOnChargerModalInstance;

  [SerializeField]
  private ProfileCreationModal _ProfileCreationModalPrefab;
  private ProfileCreationModal _ProfileCreationModalInstance;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _PrivacyPolicyButton;

  [SerializeField]
  private string _PrivacyPolicyFileName;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _TermsOfUseButton;

  [SerializeField]
  private string _TermsOfUseFileName;

  private void Awake() {

    DasTracker.Instance.TrackFirstTimeConnectStarted();

    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
      _StartButton.Initialize(HandleMockButton, "start_button", "first_time_connect_dialog");
    }
    else {
      _StartButton.Initialize(HandleStartButton, "start_button", "first_time_connect_dialog");
    }

    InitializePrivacyPolicyButton();
    InitializeTermsOfUseButton();

    _StartButton.Text = Localization.Get(LocalizationKeys.kLabelStart);
    UIManager.Instance.BackgroundColorController.SetBackgroundColor(Cozmo.UI.BackgroundColorController.BackgroundColor.Yellow);
  }

  private void InitializePrivacyPolicyButton() {
    System.Action<BaseModal> privacyPolicyAlertCreated = (newModal) => {
      ScrollingTextModal scrollingTextModal = (ScrollingTextModal)newModal;
      scrollingTextModal.DASEventDialogName = "privacy_policy_view";
      scrollingTextModal.Initialize(Localization.Get(LocalizationKeys.kPrivacyPolicyTitle), Localization.ReadLocalizedTextFromFile(_PrivacyPolicyFileName));
    };

    UnityEngine.Events.UnityAction privacyPolicyPressedCallback = () => {
      UIManager.OpenModal(AlertModalLoader.Instance.ScrollingTextModalPrefab, new ModalPriorityData(), privacyPolicyAlertCreated);
    };

    _PrivacyPolicyButton.Initialize(privacyPolicyPressedCallback, "privacy_policy_button", "first_time_connect_dialog");
  }

  private void InitializeTermsOfUseButton() {
    System.Action<BaseModal> termsOfUseModalCreated = (newModal) => {
      ScrollingTextModal scrollingTextModal = (ScrollingTextModal)newModal;
      scrollingTextModal.DASEventDialogName = "terms_of_use_view";
      scrollingTextModal.Initialize(Localization.Get(LocalizationKeys.kLabelTermsOfUse), Localization.ReadLocalizedTextFromFile(_TermsOfUseFileName));
    };

    UnityEngine.Events.UnityAction termsOfUseButtonPressed = () => {
      UIManager.OpenModal(AlertModalLoader.Instance.ScrollingTextModalPrefab, new ModalPriorityData(), termsOfUseModalCreated);
    };

    _TermsOfUseButton.Initialize(termsOfUseButtonPressed, "terms_of_use_button", "first_time_connect_dialog");
  }

  private void OnDestroy() {
    if (_PlaceCozmoOnChargerModalInstance != null) {
      UIManager.CloseModalImmediately(_PlaceCozmoOnChargerModalInstance);
    }

    if (_ConnectionFlowInstance != null) {
      GameObject.Destroy(_ConnectionFlowInstance.gameObject);
    }

    DasTracker.Instance.TrackFirstTimeConnectEnded();
  }

  private void HandleStartButton() {
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileCreated) {
      if (DebugMenuManager.Instance.DemoMode) {
        StartConnectionFlow();
      }
      else {
        ShowPlaceCozmoOnCharger();
      }
    }
    else {
      UIManager.OpenModal(_SoundCheckModalPrefab, new ModalPriorityData(), HandleSoundCheckViewCreated);
    }
  }

  private void HandleSoundCheckViewCreated(BaseModal newSoundCheckModal) {
    _SoundCheckModalInstance = (SoundCheckModal)newSoundCheckModal;
    _SoundCheckModalInstance.OnSoundCheckComplete += HandleSoundCheckComplete;
  }

  private void HandleSoundCheckComplete() {
    _SoundCheckModalInstance.OnSoundCheckComplete -= HandleSoundCheckComplete;
    _SoundCheckModalInstance.DialogClosed += ShowProfileCreationScreen;
    UIManager.CloseModal(_SoundCheckModalInstance);
  }

  private void ShowProfileCreationScreen() {
    UIManager.OpenModal(_ProfileCreationModalPrefab, new ModalPriorityData(), HandleProfileCreationViewCreated);
  }

  private void HandleProfileCreationViewCreated(BaseModal newProfileCreationModal) {
    _ProfileCreationModalInstance = (ProfileCreationModal)newProfileCreationModal;
    _ProfileCreationModalInstance.DialogClosed += HandleProfileCreationDone;
  }

  private void HandleProfileCreationDone() {
    _ProfileCreationModalInstance.DialogCloseAnimationFinished += ShowPlaceCozmoOnCharger;
    _ProfileCreationModalInstance.DialogClosed -= HandleProfileCreationDone;
    UIManager.CloseModal(_ProfileCreationModalInstance);
  }

  private void ShowPlaceCozmoOnCharger() {
    UIManager.OpenModal(_PlaceCozmoOnChargerModalPrefab, new ModalPriorityData(), HandlePlaceCozmoOnChargerViewCreated);
  }

  private void HandlePlaceCozmoOnChargerViewCreated(BaseModal newPlaceCozmoOnChargerModal) {
    _PlaceCozmoOnChargerModalInstance = (PlaceCozmoOnChargerModal)newPlaceCozmoOnChargerModal;
    _PlaceCozmoOnChargerModalInstance.OnConnectButton += HandleConnectButton;
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
    _PlaceCozmoOnChargerModalInstance.DialogCloseAnimationFinished += StartConnectionFlow;
    _PlaceCozmoOnChargerModalInstance.OnConnectButton -= HandleConnectButton;
    UIManager.CloseModal(_PlaceCozmoOnChargerModalInstance);
  }

  private void StartConnectionFlow() {
    _ConnectionFlowInstance = GameObject.Instantiate(_ConnectionFlowPrefab.gameObject).GetComponent<ConnectionFlowController>();
    _ConnectionFlowInstance.ConnectionFlowComplete += HandleConnectionFlowComplete;
    _ConnectionFlowInstance.ConnectionFlowQuit += HandleConnectionFlowQuit;
    _ConnectionFlowInstance.StartConnectionFlow();
  }

  private void HandleConnectionFlowComplete() {
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

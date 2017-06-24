using UnityEngine;
using Cozmo.UI;

public class PlaceCozmoOnChargerModal : BaseModal {

  public System.Action OnConnectButton;

  [SerializeField]
  private SimpleConnectModal _KeepCozmoOnChargerConnectModalPrefab;
  private SimpleConnectModal _KeepCozmoOnChargerConnectModalInstance;

  [SerializeField]
  private CozmoButton _ContinueButton;

  private void Awake() {
    System.Action<BaseModal> keepOnChargerModalCreated = (newSimpleConnectModal) => {
      _KeepCozmoOnChargerConnectModalInstance = (SimpleConnectModal)newSimpleConnectModal;
      _KeepCozmoOnChargerConnectModalInstance.OnConnectButton += HandleConnectButton;
    };

    UnityEngine.Events.UnityAction continueButtonPressed = () => {
      UIManager.OpenModal(_KeepCozmoOnChargerConnectModalPrefab,
                          ModalPriorityData.CreateSlightlyHigherData(this.PriorityData),
                          keepOnChargerModalCreated);
    };

    _ContinueButton.Initialize(continueButtonPressed, "continue_button", this.DASEventDialogName);
    DasTracker.Instance.TrackChargerPromptEntered();
  }

  private void HandleConnectButton() {
    if (OnConnectButton != null) {
      DasTracker.Instance.TrackChargerPromptConnect();
      OnConnectButton();
    }
  }

  protected override void CleanUp() {
    base.CleanUp();
    if (_KeepCozmoOnChargerConnectModalInstance != null) {
      UIManager.CloseModalImmediately(_KeepCozmoOnChargerConnectModalInstance);
    }
  }
}

using UnityEngine;
using Cozmo.UI;

public class WifiInstructionsModal : Cozmo.UI.BaseModal {

  [SerializeField]
  private CozmoButton _HelpButton;

  [SerializeField]
  private BaseModal _WifiGetHelpModalPrefab;
  private BaseModal _WifiGetHelpModalInstance;

  private void Awake() {
    _HelpButton.Initialize(() => {
      UIManager.OpenModal(_WifiGetHelpModalPrefab,
                          ModalPriorityData.CreateSlightlyHigherData(this.PriorityData),
                          (newHelpModal) => {
                            _WifiGetHelpModalInstance = newHelpModal;
                          });
      DasTracker.Instance.TrackWifiInstructionsGetHelp();
    }, "help_button", this.DASEventDialogName);
    DasTracker.Instance.TrackWifiInstructionsStarted();
  }

  protected override void CleanUp() {
    if (_WifiGetHelpModalInstance != null) {
      UIManager.CloseModalImmediately(_WifiGetHelpModalInstance);
    }
  }

  void OnDisable() {
    // this can go in CleanUp() but that is currently called twice
    DasTracker.Instance.TrackWifiInstructionsEnded();
  }
}

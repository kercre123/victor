using UnityEngine;

public class WifiInstructionsView : Cozmo.UI.BaseModal {

  [SerializeField]
  private Cozmo.UI.CozmoButton _HelpButton;

  [SerializeField]
  private Cozmo.UI.BaseModal _GetHelpViewPrefab;
  private Cozmo.UI.BaseModal _GetHelpViewInstance;

  private void Awake() {
    _HelpButton.Initialize(() => {
      _GetHelpViewInstance = UIManager.OpenModal(_GetHelpViewPrefab);
      DasTracker.Instance.TrackWifiInstructionsGetHelp();
    }, "help_button", this.DASEventViewName);
    DasTracker.Instance.TrackWifiInstructionsStarted();
  }

  protected override void CleanUp() {
    if (_GetHelpViewInstance != null) {
      UIManager.CloseModalImmediately(_GetHelpViewInstance);
    }
  }

  void OnDisable() {
    // this can go in CleanUp() but that is currently called twice
    DasTracker.Instance.TrackWifiInstructionsEnded();
  }
}

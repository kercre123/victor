using UnityEngine;

public class WifiInstructionsView : Cozmo.UI.BaseView {

  [SerializeField]
  private Cozmo.UI.CozmoButton _HelpButton;

  [SerializeField]
  private Cozmo.UI.BaseView _GetHelpViewPrefab;
  private Cozmo.UI.BaseView _GetHelpViewInstance;

  private void Awake() {
    _HelpButton.Initialize(() => {
      _GetHelpViewInstance = UIManager.OpenView(_GetHelpViewPrefab);
      DasTracker.Instance.OnWifiInstructionsGetHelp();
    }, "help_button", this.DASEventViewName);
    DasTracker.Instance.OnWifiInstructionsStarted();
  }

  protected override void CleanUp() {
    if (_GetHelpViewInstance != null) {
      UIManager.CloseViewImmediately(_GetHelpViewInstance);
    }
  }

  void OnDisable() {
    // this can go in CleanUp() but that is currently called twice
    DasTracker.Instance.OnWifiInstructionsEnded();
  }
}

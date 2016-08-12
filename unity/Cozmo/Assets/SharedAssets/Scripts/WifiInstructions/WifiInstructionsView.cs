using UnityEngine;

public class WifiInstructionsView : Cozmo.UI.BaseView {

  [SerializeField]
  private Cozmo.UI.CozmoButton _HelpButton;

  [SerializeField]
  private Cozmo.UI.BaseView _GetHelpViewPrefab;
  private Cozmo.UI.BaseView _GetHelpViewInstance;

  [SerializeField]
  private Cozmo.UI.BaseView _ReplaceCozmoOnChargerViewPrefab;
  private Cozmo.UI.BaseView _ReplaceCozmoOnChargerViewInstance;

  [SerializeField]
  private float _ReplaceCozmoViewTimeout = 25.0f;

  private void Awake() {
    _HelpButton.Initialize(() => {
      _GetHelpViewInstance = UIManager.OpenView(_GetHelpViewPrefab);
    }, "help_button", this.DASEventViewName);

    Invoke("OpenReplaceCozmoView", _ReplaceCozmoViewTimeout);
  }

  private void OpenReplaceCozmoView() {
    _ReplaceCozmoOnChargerViewInstance = UIManager.OpenView(_ReplaceCozmoOnChargerViewPrefab);
  }

  protected override void CleanUp() {
    if (_GetHelpViewInstance != null) {
      UIManager.CloseViewImmediately(_GetHelpViewInstance);
    }

    if (_ReplaceCozmoOnChargerViewInstance != null) {
      UIManager.CloseViewImmediately(_ReplaceCozmoOnChargerViewInstance);
    }
  }
}

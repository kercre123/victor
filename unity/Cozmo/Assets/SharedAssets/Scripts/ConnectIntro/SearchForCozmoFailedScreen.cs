using UnityEngine;
using System.Collections;

public class SearchForCozmoFailedScreen : MonoBehaviour {

  public System.Action OnEndpointFound;
  public System.Action OnQuitFlow;

  [SerializeField]
  private Cozmo.UI.CozmoButton _QuitButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ShowMeButton;

  [SerializeField]
  private WifiInstructionsView _WifiInstructionsViewPrefab;
  private WifiInstructionsView _WifiInstructionsViewInstance;

  private PingStatus _PingStatus;

  private void OnDestroy() {
    if (_WifiInstructionsViewInstance != null) {
      UIManager.CloseView(_WifiInstructionsViewInstance);
    }
  }

  public void Initialize(PingStatus pingStatus) {
    _PingStatus = pingStatus;
  }

  private void Start() {
    _QuitButton.Initialize(HandleQuitButton, "wifi_instructions_quit_button", "search_for_cozmo_failed_screen");
    _ShowMeButton.Initialize(HandleShowMeButton, "show_me_button", "search_for_cozmo_failed_screen");
  }

  private void Update() {
    if (_PingStatus.GetPingStatus()) {
      if (OnEndpointFound != null) {
        OnEndpointFound();
      }
    }
  }

  private void HandleQuitButton() {
    if (OnQuitFlow != null) {
      OnQuitFlow();
    }
  }

  private void HandleShowMeButton() {
    _WifiInstructionsViewInstance = UIManager.OpenView(_WifiInstructionsViewPrefab);
  }

}

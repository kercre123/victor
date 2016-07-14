using UnityEngine;
using System.Collections;

public class SearchForCozmoFailedScreen : MonoBehaviour {

  public System.Action OnEndpointFound;
  public System.Action OnQuitFlow;

  [SerializeField]
  private Cozmo.UI.CozmoButton _QuitButton;

  private PingStatus _PingStatus;

  public void Initialize(PingStatus pingStatus) {
    _PingStatus = pingStatus;
  }

  private void Start() {
    _QuitButton.Initialize(HandleQuitButton, "wifi_instructions_quit_button", "search_for_cozmo_failed_screen");
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

}

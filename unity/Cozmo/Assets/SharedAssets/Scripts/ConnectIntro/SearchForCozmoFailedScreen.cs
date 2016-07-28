using UnityEngine;
using System.Collections;

public class SearchForCozmoFailedScreen : MonoBehaviour {

  public System.Action OnEndpointFound;
  public System.Action OnQuitFlow;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ShowMeButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _GetACozmoButton;

  [SerializeField]
  private WifiInstructionsView _WifiInstructionsViewPrefab;
  private WifiInstructionsView _WifiInstructionsViewInstance;

  private PingStatus _PingStatus;

  public void Initialize(PingStatus pingStatus) {
    _PingStatus = pingStatus;
  }

  private void Awake() {
    _ShowMeButton.Initialize(HandleShowMeButton, "show_me_button", "search_for_cozmo_failed_screen");
    _GetACozmoButton.Initialize(HandleGetACozmoButton, "get_a_cozmo_button", "search_for_cozmo_failed_screen");
  }

  private void OnDestroy() {
    if (_WifiInstructionsViewInstance != null) {
      UIManager.CloseViewImmediately(_WifiInstructionsViewInstance);
    }
  }

  private void HandleGetACozmoButton() {
    Application.OpenURL("https://www.anki.com");
  }

  private void Update() {
    if (_PingStatus.GetPingStatus()) {
      if (OnEndpointFound != null) {
        OnEndpointFound();
      }
    }
  }

  private void QuitFlow() {
    if (OnQuitFlow != null) {
      OnQuitFlow();
    }
  }

  private void HandleShowMeButton() {
    _WifiInstructionsViewInstance = UIManager.OpenView(_WifiInstructionsViewPrefab);
    _WifiInstructionsViewInstance.ViewClosedByUser += QuitFlow;
  }

}

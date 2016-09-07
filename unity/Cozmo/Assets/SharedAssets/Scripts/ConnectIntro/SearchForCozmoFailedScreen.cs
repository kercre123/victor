using UnityEngine;
using System.Collections;

public class SearchForCozmoFailedScreen : MonoBehaviour {

  private const int kDeviceIdLength = 13;
  public System.Action OnEndpointFound;
  public System.Action OnQuitFlow;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ShowMeButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _GetACozmoButton;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _LastCozmoLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _DeviceIdLabel;

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

    var persistence = DataPersistence.DataPersistenceManager.Instance;
    var lastCozmoSerial = persistence.Data.DeviceSettings.LastCozmoSerial;
    if (lastCozmoSerial != 0) {
      _LastCozmoLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelCozmoSerial, lastCozmoSerial.ToString("X"));
    } else {
      _LastCozmoLabel.gameObject.SetActive(false);
    }

    var deviceId = persistence.DeviceId;
    if (!string.IsNullOrEmpty(deviceId)) {
      deviceId = deviceId.Substring(0, System.Math.Min(kDeviceIdLength, deviceId.Length)); // 12 chars of device id + one dash in the middle
      _DeviceIdLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelDeviceid, deviceId);   
    } else {
      _DeviceIdLabel.gameObject.SetActive(false);
    }
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

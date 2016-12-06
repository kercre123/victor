using UnityEngine;
using System.Collections;
using Anki.Cozmo.ExternalInterface;


public class SearchForCozmoFailedScreen : MonoBehaviour {
  public System.Action OnEndpointFound;
  public System.Action OnQuitFlow;

  private const int kAppVerCharsToDisplay = 5;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ShowMeButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _GetACozmoButton;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _LastCozmoLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _DeviceIdLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _AppVerLabel;

  [SerializeField]
  private WifiInstructionsView _WifiInstructionsViewPrefab;
  private WifiInstructionsView _WifiInstructionsViewInstance;

  [SerializeField]
  private GameObject _WifiAnimationsPrefab;

  private PingStatus _PingStatus;

  public void Initialize(PingStatus pingStatus) {
    _PingStatus = pingStatus;
  }

  private void Awake() {
    _ShowMeButton.Initialize(HandleShowMeButton, "show_me_button", "search_for_cozmo_failed_screen");
    _GetACozmoButton.Initialize(HandleGetACozmoButton, "get_a_cozmo_button", "search_for_cozmo_failed_screen");
    RobotEngineManager.Instance.AddCallback<DeviceDataMessage>(HandleDeviceDataMessage);
    RobotEngineManager.Instance.SendRequestDeviceData();

    GameObject wifiAnimations = GameObject.Instantiate(_WifiAnimationsPrefab);
    wifiAnimations.transform.SetParent(transform, false);

    Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect_Fail);

    var persistence = DataPersistence.DataPersistenceManager.Instance;
    var lastCozmoSerial = persistence.Data.DeviceSettings.LastCozmoSerial;
    if (lastCozmoSerial != 0) {
      _LastCozmoLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelCozmoSerial, lastCozmoSerial.ToString("X"));
    }
    else {
      _LastCozmoLabel.gameObject.SetActive(false);
    }

    var deviceId = persistence.DeviceId;
    if (!string.IsNullOrEmpty(deviceId)) {
      deviceId = deviceId.Substring(0, System.Math.Min(Cozmo.Settings.DefaultSettingsValuesConfig.Instance.CharactersOfAppInfoToShow,
                                                       deviceId.Length)); // 12 chars of device id + one dash in the middle
      _DeviceIdLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelDeviceid, deviceId);
    }
    else {
      _DeviceIdLabel.gameObject.SetActive(false);
    }

    DasTracker.Instance.TrackSearchForCozmoFailed();
  }

  // Set up our BuildVersion to be based on the DeviceData received
  private void HandleDeviceDataMessage(DeviceDataMessage message) {
    for (int i = 0; i < message.dataList.Length; ++i) {
      Anki.Cozmo.DeviceDataPair currentPair = message.dataList[i];
      if (currentPair.dataType == Anki.Cozmo.DeviceDataType.BuildVersion) {
        string appVer = currentPair.dataValue;
        if (appVer.Length > kAppVerCharsToDisplay) {
          appVer = appVer.Substring(0, kAppVerCharsToDisplay);
        }
        _AppVerLabel.FormattingArgs = new object[] { appVer };
      }
    }

  }

  private void OnDestroy() {
    if (_WifiInstructionsViewInstance != null) {
      UIManager.CloseModalImmediately(_WifiInstructionsViewInstance);
    }
    RobotEngineManager.Instance.RemoveCallback<DeviceDataMessage>(HandleDeviceDataMessage);
  }

  private void HandleGetACozmoButton() {
    Application.OpenURL(Cozmo.Settings.DefaultSettingsValuesConfig.Instance.GetACozmoURL);
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
    _WifiInstructionsViewInstance = UIManager.OpenModal(_WifiInstructionsViewPrefab);
    _WifiInstructionsViewInstance.ViewClosedByUser += QuitFlow;
  }

}

using UnityEngine;
using System.Collections;

public class FirmwarePane : MonoBehaviour {

  [SerializeField]
  private Anki.UI.AnkiTextLabel _OutputText;
  [SerializeField]
  private Anki.UI.AnkiButton _UpgradeButton;

  void Start() {
    _UpgradeButton.DASEventButtonName = "debug_upgrade_firmware_button";
    _UpgradeButton.DASEventViewController = "debug_upgrade_firmware_view";
    _UpgradeButton.onClick.AddListener(() => RobotEngineManager.Instance.UpdateFirmware(0));

    RobotEngineManager.Instance.OnFirmwareUpdateProgress += OnFirmwareUpdateProgress;
    RobotEngineManager.Instance.OnFirmwareUpdateComplete += OnFirmwareUpdateComplete;
  }

  public void OnDestroy() {
    RobotEngineManager.Instance.OnFirmwareUpdateProgress -= OnFirmwareUpdateProgress;
    RobotEngineManager.Instance.OnFirmwareUpdateComplete -= OnFirmwareUpdateComplete;
  }


  private void OnFirmwareUpdateProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    _OutputText.text = "InProgress: Robot " + message.robotID + " Stage: " + message.stage + ":" + message.subStage + " " + message.percentComplete + "%"
    + "\nWifi = " + message.fwSigWifi + "\nRTIP = " + message.fwSigRTIP + "\nBody = " + message.fwSigBody;
  }

  private void OnFirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    _OutputText.text = "Complete: Robot " + message.robotID + " Result: " + message.result
    + "\nWifi = " + message.fwSigWifi + "\nRTIP = " + message.fwSigRTIP + "\nBody = " + message.fwSigBody;
  }


}

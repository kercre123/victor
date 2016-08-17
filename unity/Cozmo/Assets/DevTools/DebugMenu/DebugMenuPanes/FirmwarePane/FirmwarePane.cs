using Anki.Cozmo;
using UnityEngine;
using System.Collections;

public class FirmwarePane : MonoBehaviour {

  [SerializeField]
  private Anki.UI.AnkiTextLabel _OutputText;
  [SerializeField]
  private Anki.UI.AnkiButton _UpgradeButton;
  [SerializeField]
  private Anki.UI.AnkiButton _DowngradeButton;
  [SerializeField]
  private Anki.UI.AnkiButton _ResetButton;

  void Start() {
    _UpgradeButton.Initialize(() => RobotEngineManager.Instance.UpdateFirmware(FirmwareType.Current, 0), "debug_upgrade_firmware_button", "debug_firmware_view");
    _DowngradeButton.Initialize(() => RobotEngineManager.Instance.UpdateFirmware(FirmwareType.Old, 0), "debug_downgrade_firmware_button", "debug_firmware_view");
    _ResetButton.Initialize(() => RobotEngineManager.Instance.ResetFirmware(), "debug_reset_firmware_button", "debug_firmware_view");

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(OnFirmwareUpdateProgress);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(OnFirmwareUpdateComplete);
  }

  public void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(OnFirmwareUpdateProgress);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(OnFirmwareUpdateComplete);
  }


  private void OnFirmwareUpdateProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    _OutputText.text = "InProgress: Robot " + message.robotID + " Stage: " + message.stage + ":" + message.subStage + " " + message.percentComplete + "%"
    + "\nFwSig = " + message.fwSig;
  }

  private void OnFirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    _OutputText.text = "Complete: Robot " + message.robotID + " Result: " + message.result
    + "\nFwSig = " + message.fwSig;
  }


}

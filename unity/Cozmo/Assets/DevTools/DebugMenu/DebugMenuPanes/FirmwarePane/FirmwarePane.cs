using UnityEngine;
using System.Collections;

public class FirmwarePane : MonoBehaviour {

  [SerializeField]
  private Anki.UI.AnkiTextLabel _OutputText;
  [SerializeField]
  private Anki.UI.AnkiButton _UpgradeButton;

  void Start() {
    _UpgradeButton.onClick.AddListener(() => RobotEngineManager.Instance.UpdateFirmware(0));

    RobotEngineManager.Instance.OnFirmwareUpdateProgress += OnFirmwareUpdateProgress;
    RobotEngineManager.Instance.OnFirmwareUpdateComplete += OnFirmwareUpdateComplete;
  }

  public void OnDestroy() {
    RobotEngineManager.Instance.OnFirmwareUpdateProgress -= OnFirmwareUpdateProgress;
    RobotEngineManager.Instance.OnFirmwareUpdateComplete -= OnFirmwareUpdateComplete;
  }


  private void OnFirmwareUpdateProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    _OutputText.text = "InProgress: Robot " + message.robotID + " Stage: " + message.stage + ":" + message.subStage + " " + message.percentComplete + "%";
  }

  private void OnFirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    _OutputText.text = "Complete: Robot " + message.robotID + " Result: " + message.result;
  }


}

using UnityEngine;
using System.Collections;

public class UpdateFirmwareScreen : MonoBehaviour {

  public System.Action<bool> FirmwareUpdateDone;

  [SerializeField]
  private Cozmo.UI.ProgressBar _ProgressBar;

  private void Start() {
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(FirmwareUpdateComplete);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(HandleFirmwareProgress);
    RobotEngineManager.Instance.UpdateFirmware(0);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(FirmwareUpdateComplete);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(HandleFirmwareProgress);
  }

  private void HandleFirmwareProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    _ProgressBar.SetProgress(message.percentComplete / 100.0f);
  }

  private void FirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    if (FirmwareUpdateDone != null) {
      if (message.result == Anki.Cozmo.FirmwareUpdateResult.Success) {
        FirmwareUpdateDone(true);
      }
      else {
        DAS.Error("UpdateFirmwareScreen.FirmwareUpdateComplete", "Firmware Update Failed: " + message.result);
        FirmwareUpdateDone(false);
      }

    }
  }
}

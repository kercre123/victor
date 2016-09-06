using UnityEngine;
using System.Collections;

public class UpdateFirmwareScreen : MonoBehaviour {

  public System.Action<bool> FirmwareUpdateDone;

  [SerializeField]
  private Cozmo.UI.ProgressBar _ProgressBar;

  [SerializeField]
  private float _DoneUpdateDelay = 10.0f;

  public bool DoneUpdateDelayInProgress = false;

  private void Start() {
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(FirmwareUpdateComplete);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(HandleFirmwareProgress);
    RobotEngineManager.Instance.UpdateFirmware(Anki.Cozmo.FirmwareType.Current, 0);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(FirmwareUpdateComplete);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(HandleFirmwareProgress);
  }

  private void HandleFirmwareProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    // ignore the other substate progression messages
    if (message.subStage == Anki.Cozmo.FirmwareUpdateSubStage.Flash) {
      DAS.Debug("UpdateFirmwareScreen.HandleFirmwareProgress", message.percentComplete.ToString());
      _ProgressBar.SetProgress(message.percentComplete / 100.0f);
    }
  }

  private void FirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    DoneUpdateDelayInProgress = true;
    StartCoroutine(NotifyFirmwareComplete(message));
  }

  private IEnumerator NotifyFirmwareComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {

    yield return new WaitForSeconds(_DoneUpdateDelay);

    if (FirmwareUpdateDone != null) {
      if (message.result == Anki.Cozmo.FirmwareUpdateResult.Success) {
        FirmwareUpdateDone(true);
      }
      else {
        DAS.Error("UpdateFirmwareScreen.FirmwareUpdateComplete", "Firmware Update Failed: " + message.result);
        FirmwareUpdateDone(false);
      }
    }

    DoneUpdateDelayInProgress = false;
  }
}

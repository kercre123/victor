using UnityEngine;
using System.Collections;

public class UpdateFirmwareScreen : MonoBehaviour {

  public System.Action<bool> FirmwareUpdateDone;

  [SerializeField]
  private Cozmo.UI.ProgressBar _ProgressBar;

  [SerializeField]
  private float _DoneUpdateDelay = 10.0f;

  public bool DoneUpdateDelayInProgress = false;
  private float _StartDelayTime = 0.0f;

  private void Start() {
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(FirmwareUpdateComplete);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(HandleFirmwareProgress);
    RobotEngineManager.Instance.UpdateFirmware(Anki.Cozmo.FirmwareType.Current, 0);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(FirmwareUpdateComplete);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(HandleFirmwareProgress);
    StopAllCoroutines();
  }

  private void HandleFirmwareProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    // ignore the other substate progression messages
    if (message.subStage == Anki.Cozmo.FirmwareUpdateSubStage.Flash) {
      DAS.Debug("UpdateFirmwareScreen.HandleFirmwareProgress", message.percentComplete.ToString());
      _ProgressBar.SetProgress((message.percentComplete / 100.0f) * 0.9f);
    }
  }

  private void FirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    if (DoneUpdateDelayInProgress) {
      return;
    }
    if (message.result != Anki.Cozmo.FirmwareUpdateResult.Success) {
      DAS.Error("UpdateFirmwareScreen.FirmwareUpdateComplete", "Firmware Update Failed: " + message.result);
      if (FirmwareUpdateDone != null) {
        FirmwareUpdateDone(false);
      }
      return;
    }

    // successful so lets add the delay reboot
    DoneUpdateDelayInProgress = true;
    _StartDelayTime = Time.time;
    StartCoroutine(NotifyFirmwareComplete(message));
  }

  private IEnumerator NotifyFirmwareComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    while (Time.time - _StartDelayTime < _DoneUpdateDelay) {
      float delayPercentage = ((Time.time - _StartDelayTime) / _DoneUpdateDelay);
      _ProgressBar.SetProgress(0.9f + delayPercentage * 0.1f);
      yield return null;
    }

    _ProgressBar.SetProgress(1.0f);

    if (FirmwareUpdateDone != null) {
      FirmwareUpdateDone(true);
    }

    DoneUpdateDelayInProgress = false;
  }
}

using UnityEngine;
using System.Collections;

public class UpdateFirmwareScreen : MonoBehaviour {

  public System.Action<bool> FirmwareUpdateDone;

  [SerializeField]
  private Cozmo.UI.ProgressBar _ProgressBar;

  [SerializeField]
  private float _DoneUpdateDelay = 7.0f;

  // Delay after which we disconnect following an OTA. This is to ensure that we DO disconnect (instead of holding onto a stale connection
  // when the robot reboots really quickly), but delayed so that slower OTAs (like from factory firmware) aren't interrupted when we attempt
  // to reconnect afterward. Also should be shorter than the 5 second reliable transport timeout that will auto disconnect after the slower OTAs.
  private const float _kDelayBeforeDisconnect_s = 4.0f;

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
    bool disconnectHasTriggered = false;
    float currentTimeElapsed_s = Time.time - _StartDelayTime;
    while (currentTimeElapsed_s < _DoneUpdateDelay) {
      
      float delayPercentage = (currentTimeElapsed_s / _DoneUpdateDelay);
      _ProgressBar.SetProgress(0.9f + delayPercentage * 0.1f);


      if (!disconnectHasTriggered && currentTimeElapsed_s >= _kDelayBeforeDisconnect_s) {
        RobotEngineManager.Instance.StartIdleTimeout(faceOffTime_s: -1.0f, disconnectTime_s: 0.0f);
        disconnectHasTriggered = true;
      }
        
      yield return null;
      currentTimeElapsed_s = Time.time - _StartDelayTime;
    }


    _ProgressBar.SetProgress(1.0f);

    if (FirmwareUpdateDone != null) {
      FirmwareUpdateDone(true);
    }

    DoneUpdateDelayInProgress = false;
  }
}

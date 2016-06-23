using UnityEngine;
using System.Collections;

#if FACTORY_TEST

public class FactoryOTAPanel : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Text _OTAStatus;

  [SerializeField]
  private UnityEngine.UI.Button _RestartButton;

  public System.Action OnFirmwareComplete;
  public System.Action OnRestartButton;

  void Start() {
    RobotEngineManager.Instance.AddCallback(typeof(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress), OnFirmwareUpdateProgress);
    RobotEngineManager.Instance.AddCallback(typeof(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete), OnFirmwareUpdateComplete);
    _RestartButton.gameObject.SetActive(false);
    _RestartButton.onClick.AddListener(HandleRestartButton);
  }

  private void OnFirmwareUpdateProgress(object messageObject) {
    Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message = (Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress)messageObject;
    _OTAStatus.text = "InProgress: Robot " + message.robotID + " Stage: " + message.stage + ":" + message.subStage + " " + message.percentComplete + "%"
    + "\nFwSig = " + message.fwSig;
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback(typeof(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress), OnFirmwareUpdateProgress);
    RobotEngineManager.Instance.RemoveCallback(typeof(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete), OnFirmwareUpdateComplete);
  }

  private void OnFirmwareUpdateComplete(object messageObject) {
    Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message = (Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete)messageObject;
    _OTAStatus.text = "Complete: Robot " + message.robotID + " Result: " + message.result
    + "\nFwSig = " + message.fwSig;

    _RestartButton.gameObject.SetActive(true);

    if (OnFirmwareComplete != null) {
      OnFirmwareComplete();
    }
  }

  private void HandleRestartButton() {
    if (OnRestartButton != null) {
      OnRestartButton();
    }
  }


}

#endif
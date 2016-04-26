using UnityEngine;
using System.Collections;

public class FactoryOTAPanel : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Text _OTAStatus;

  public System.Action OnFirmwareComplete;

  void Start() {
    RobotEngineManager.Instance.OnFirmwareUpdateProgress += OnFirmwareUpdateProgress;
    RobotEngineManager.Instance.OnFirmwareUpdateComplete += OnFirmwareUpdateComplete;
  }

  private void OnFirmwareUpdateProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    _OTAStatus.text = "InProgress: Robot " + message.robotID + " Stage: " + message.stage + ":" + message.subStage + " " + message.percentComplete + "%"
    + "\nWifi = " + message.fwSigWifi + "\nRTIP = " + message.fwSigRTIP + "\nBody = " + message.fwSigBody;
  }

  private void OnFirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    if (OnFirmwareComplete != null) {
      OnFirmwareComplete();
    }
  }


}

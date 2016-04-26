using UnityEngine;
using System.Collections;

public class FactoryOTAPanel : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Text _OTAStatus;

  [SerializeField]
  private UnityEngine.UI.Button _RestartButton;

  public System.Action OnFirmwareComplete;
  public System.Action OnRestartButton;

  void Start() {
    RobotEngineManager.Instance.OnFirmwareUpdateProgress += OnFirmwareUpdateProgress;
    RobotEngineManager.Instance.OnFirmwareUpdateComplete += OnFirmwareUpdateComplete;
    _RestartButton.gameObject.SetActive(false);
    _RestartButton.onClick.AddListener(HandleRestartButton);
  }

  private void OnFirmwareUpdateProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    _OTAStatus.text = "InProgress: Robot " + message.robotID + " Stage: " + message.stage + ":" + message.subStage + " " + message.percentComplete + "%"
    + "\nWifi = " + message.fwSigWifi + "\nRTIP = " + message.fwSigRTIP + "\nBody = " + message.fwSigBody;
  }

  private void OnFirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    _OTAStatus.text = "Complete: Robot " + message.robotID + " Result: " + message.result
    + "\nWifi = " + message.fwSigWifi + "\nRTIP = " + message.fwSigRTIP + "\nBody = " + message.fwSigBody;

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

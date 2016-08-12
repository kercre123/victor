using UnityEngine;
using System.Collections;


#if FACTORY_TEST

public class FactoryOTAPanel : MonoBehaviour {

  public System.Action OnOTAStarted;
  public System.Action OnOTAFinished;

  [SerializeField]
  private UnityEngine.UI.Text _OTAStatus;

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  void Start() {
    if (OnOTAStarted != null) {
      OnOTAStarted();
    }

    RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToClient;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(HandleRobotConnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleRobotDisconnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(OnFirmwareUpdateProgress);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(OnFirmwareUpdateComplete);

    if (!RobotEngineManager.Instance.IsConnectedToEngine) {
      _OTAStatus.text = "Connecting to engine";
      RobotEngineManager.Instance.Connect(FactoryIntroManager.kEngineIP);
    }
    else {
      HandleConnectedToClient(null);
    }

    _CloseButton.onClick.AddListener(HandleCloseButton);
  }

  private void HandleConnectedToClient(string connectionIdentifier) {
    _OTAStatus.text = "Connecting to robot";
    RobotEngineManager.Instance.ConnectToRobot(FactoryIntroManager.kRobotID, FactoryIntroManager.kRobotIP, false);
  }

  private void HandleDisconnectedFromClient(DisconnectionReason obj) {
    _OTAStatus.text = "Disconnected from engine";
  }

  private void HandleRobotConnected(Anki.Cozmo.ExternalInterface.RobotConnectionResponse message) {
    _OTAStatus.text = "Sending update firmware message";
    RobotEngineManager.Instance.UpdateFirmware(-1);
  }

  private void HandleRobotDisconnected(Anki.Cozmo.ExternalInterface.RobotDisconnected message) {
    _OTAStatus.text = "Disconnected from robot";
  }

  private void OnFirmwareUpdateProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    _OTAStatus.text = "InProgress: Robot " + message.robotID + " Stage: " + message.stage + ":" + message.subStage + " " + message.percentComplete + "%"
    + "\nFwSig = " + message.fwSig;
  }

  private void OnFirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    _OTAStatus.text = "Complete: Robot " + message.robotID + " Result: " + message.result + "\nFwSig = " + message.fwSig;
  }

  private void HandleCloseButton() {
    RobotEngineManager.Instance.DisconnectFromRobot(FactoryIntroManager.kRobotID);

    RobotEngineManager.Instance.ConnectedToClient -= HandleConnectedToClient;
    RobotEngineManager.Instance.DisconnectedFromClient -= HandleDisconnectedFromClient;

    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(HandleRobotConnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleRobotDisconnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(OnFirmwareUpdateProgress);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(OnFirmwareUpdateComplete);

    if (OnOTAFinished != null) {
      OnOTAFinished();
    }

    GameObject.Destroy(gameObject);
  }
}

#endif
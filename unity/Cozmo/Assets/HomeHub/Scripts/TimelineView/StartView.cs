using UnityEngine;
using System.Collections;
using Anki.UI;
using UnityEngine.UI;
using Cozmo.UI;

public class StartView : BaseView {

  [SerializeField]
  private AnkiButton _ConnectButton;

  [SerializeField]
  private Color _DisconnectedColor;

  [SerializeField]
  private Image _WifiIndicator;

  [SerializeField]
  private Image _BluetoothIndicator;

  [SerializeField]
  private Button _SecretSkipButton;

  public event System.Action OnConnectClicked;

  private void Awake() {
    _ConnectButton.onClick.AddListener(HandleConnectClicked);
    _SecretSkipButton.onClick.AddListener(HandleSecretSkipButtonClicked);
    LoopRobotSleep();
  }

  private bool IsWifiConnected() {
    return Application.internetReachability == NetworkReachability.ReachableViaLocalAreaNetwork;
  }

  private bool IsBluetoothConnected() {
    // TODO: Figure this one out.
    return true;
  }

  // we want to update our status in real time.
  private void Update() {
    _WifiIndicator.color = IsWifiConnected() ? Color.white : _DisconnectedColor;
    _BluetoothIndicator.color = IsBluetoothConnected() ? Color.white : _DisconnectedColor;
  }

  private void HandleConnectClicked() {
    var robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      DAS.Info(this, "Cancelling HandleSleepAnimationComplete");
      robot.CancelCallback(HandleSleepAnimationComplete);
      robot.SendAnimation(AnimationName.kOpenEyesTwo, HandleWakeAnimationComplete);
      _ConnectButton.Interactable = false;
    }
  }

  private void LoopRobotSleep() {
    var robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      DAS.Info(this, "Sending Sleeping Animation");
      robot.SendAnimation(AnimationName.kSleeping, HandleSleepAnimationComplete);
    }
  }

  private void HandleSleepAnimationComplete(bool success) {
    DAS.Info(this, "HandleSleepAnimationComplete: success: " + success);
    LoopRobotSleep();
  }

  private void HandleWakeAnimationComplete(bool success) {
    DAS.Info(this, "HandleWakeAnimationComplete: success: " + success);

    var robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      // Display Cozmo's default face
      robot.DisplayProceduralFace(
        0,
        Vector2.zero, 
        Vector2.one, 
        ProceduralEyeParameters.MakeDefaultLeftEye(),
        ProceduralEyeParameters.MakeDefaultRightEye());
    }

    if (OnConnectClicked != null) {
      OnConnectClicked();
    }
  }

  private void HandleSecretSkipButtonClicked() {
    // just trigger our callback. Everything should get cleaned up
    if (OnConnectClicked != null) {
      OnConnectClicked();
    }
  }

  #region implemented abstract members of BaseView

  protected override void CleanUp() {
    
    OnConnectClicked = null;
    var robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      // if the startup view got interrupted somehow, clean up our animation callbacks.
      robot.CancelCallback(HandleSleepAnimationComplete);
      robot.CancelCallback(HandleWakeAnimationComplete);
    }
  }

  #endregion
}

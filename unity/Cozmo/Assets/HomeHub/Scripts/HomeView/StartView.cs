using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;

public class StartView : BaseView {

  [SerializeField]
  private Color _DisconnectedColor;

  [SerializeField]
  private Image _WifiIndicator;

  [SerializeField]
  private Image _BluetoothIndicator;

  [SerializeField]
  private CozmoButton _SecretSkipButton;

  [SerializeField]
  private CozmoButton _ConnectButton;

  [SerializeField]
  private CanvasGroup _AlphaController;

  [SerializeField]
  private float _FadeTweenDurationSeconds = 0.3f;

  public event System.Action OnConnectClicked;

  private void Awake() {
    _ConnectButton.Initialize(HandleConnectClicked, "wake_up_cozmo_button", DASEventViewName);

#if UNITY_EDITOR
    _SecretSkipButton.Initialize(HandleSecretSkipButtonClicked, "secret_button", DASEventViewName);
#endif

    LoopRobotSleep();
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Sleep);
  }

  private void Update() {
    if (_WifiIndicator != null) {
      _WifiIndicator.color = IsWifiConnected() ? Color.white : _DisconnectedColor;
    }
    if (_BluetoothIndicator != null) {
      _BluetoothIndicator.color = IsBluetoothConnected() ? Color.white : _DisconnectedColor;
    }
  }

  private bool IsWifiConnected() {
    return Application.internetReachability == NetworkReachability.ReachableViaLocalAreaNetwork;
  }

  private bool IsBluetoothConnected() {
    // TODO: Figure this one out.
    return true;
  }

  private void HandleConnectClicked() {
    var robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      DAS.Info(this, "Cancelling HandleSleepAnimationComplete");
      robot.CancelCallback(HandleSleepAnimationComplete);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedBlockConnect);
      robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.ConnectWakeUp, HandleWakeAnimationComplete);
    }
  }

  private void LoopRobotSleep() {
    var robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      DAS.Info(this, "Sending Sleeping Animation");
      // INGO: This is a bit of a hack so that Mooly can test animations while the StartView is open
      if (!DebugMenuManager.Instance.IsDialogOpen()) {
        robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.Sleeping, HandleSleepAnimationComplete);
      }
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

  protected override void ConstructOpenAnimation(Sequence openAnimation) {
    // INGO
    // Do nothing because the ConnectDialog looks very similar; uncomment to fade in
    // openAnimation.Append(_AlphaController.DOFade(1, _FadeTweenDurationSeconds));
  }

  protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    closeAnimation.Append(_AlphaController.DOFade(0, _FadeTweenDurationSeconds));
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

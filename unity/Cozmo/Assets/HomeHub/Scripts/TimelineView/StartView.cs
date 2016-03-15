using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using UnityEngine.UI;
using Cozmo.UI;

public class StartView : BaseView {

  [SerializeField]
  private Color _DisconnectedColor;

  [SerializeField]
  private Image _WifiIndicator;

  [SerializeField]
  private Image _BluetoothIndicator;

  [SerializeField]
  private Button _SecretSkipButton;

  [SerializeField]
  private ShowCozmoCubeSlide _ShowCozmoCubesContainer;

  public event System.Action OnConnectClicked;

  private int _NumRequiredCubes = 3;
  private List<int> _CubeIdsSeen;

  private void Awake() {
    _CubeIdsSeen = new List<int>();

    #if UNITY_EDITOR
    _SecretSkipButton.onClick.AddListener(HandleSecretSkipButtonClicked);
    #endif

    LoopRobotSleep();
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Wakeup);

    _ShowCozmoCubesContainer.Initialize(_NumRequiredCubes, Cozmo.CubePalette.OutOfViewColor, Cozmo.CubePalette.InViewColor, 
      LocalizationKeys.kConnectLabelShowCubes);
  }

  private void Update() {
    _WifiIndicator.color = IsWifiConnected() ? Color.white : _DisconnectedColor;
    _BluetoothIndicator.color = IsBluetoothConnected() ? Color.white : _DisconnectedColor;

    int currentNumCubes = RobotEngineManager.Instance.CurrentRobot.LightCubes.Count;
    if (_CubeIdsSeen.Count < currentNumCubes) {
      foreach (var cube in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
        if (cube.Value.MarkersVisible && !_CubeIdsSeen.Contains(cube.Key)) {
          _CubeIdsSeen.Add(cube.Value.ID);
          cube.Value.SetLEDs(Cozmo.CubePalette.InViewColor.lightColor);
        }
      }
      _ShowCozmoCubesContainer.LightUpCubes(_CubeIdsSeen.Count);
      if (_CubeIdsSeen.Count >= _NumRequiredCubes) {
        HandleConnectClicked();
      }
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
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
      robot.SendAnimation(AnimationName.kConnect_WakeUp, HandleWakeAnimationComplete);
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

using UnityEngine;
using System.Collections;

public class PressDemoView : Cozmo.UI.BaseView {

  public System.Action OnForceProgress;
  public System.Action<bool> OnStartButton;

  [SerializeField]
  private UnityEngine.UI.Button _ForceProgressSecretButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _StartButton;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _StartButtonText;

  // this button is hidden and on top of _ForceProgressButton
  // and used if the demoer wants to start the demo and skip
  // the initial edge detection scene.
  [SerializeField]
  private UnityEngine.UI.Button _StartNoEdgeSecretButton;

  [SerializeField]
  private UnityEngine.UI.Text _FatalErrorText;

  [SerializeField]
  private UnityEngine.UI.Image _PressDebugBaseImage;

  [SerializeField]
  private Sprite[] _PressDebugDatabase;

  [SerializeField]
  private UnityEngine.UI.Button _ResetButton;

  void Start() {
    _ForceProgressSecretButton.onClick.AddListener(HandleForceProgressPressed);
    _StartButton.onClick.AddListener(HandleStartButton);
    _StartNoEdgeSecretButton.onClick.AddListener(HandleStartNoEdgeButton);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(OnClientDisconnect);
    _ResetButton.onClick.AddListener(HandleResetButton);
  }

  private void HandleResetButton() {
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      DAS.Debug("PressDemoView.HandleResetButton", "Sending Reset Button for Demo");
      robot.SendDemoResetState();
    }
  }

  private void OnClientDisconnect(object message) {
    _FatalErrorText.text = "Disconnected from Cozmo: Robot disconnected";
  }

  public void SetPressDemoDebugState(int index) {
    _PressDebugBaseImage.overrideSprite = _PressDebugDatabase[index];
  }

  public void HideStartButtons() {
    _StartButton.Interactable = false;
    _StartButtonText.text = "";
    _StartNoEdgeSecretButton.gameObject.SetActive(false);
  }

  private void HandleStartButton() {
    if (OnStartButton != null) {
      OnStartButton(true);
    }
    RobotEngineManager.Instance.CurrentRobot.WakeUp(true);
    HideStartButtons();
  }

  private void HandleStartNoEdgeButton() {
    if (OnStartButton != null) {
      OnStartButton(false);
    }
    RobotEngineManager.Instance.CurrentRobot.WakeUp(false);
    HideStartButtons();
  }

  private void HandleForceProgressPressed() {
    if (OnForceProgress != null) {
      OnForceProgress();
    }
  }

  protected override void CleanUp() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(OnClientDisconnect);
  }
}

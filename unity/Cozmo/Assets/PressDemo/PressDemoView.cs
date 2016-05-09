using UnityEngine;
using System.Collections;

public class PressDemoView : Cozmo.UI.BaseView {

  public System.Action OnForceProgress;
  public System.Action<bool> OnStartButton;

  [SerializeField]
  private UnityEngine.UI.Button _ForceProgressSecretButton;

  [SerializeField]
  private UnityEngine.UI.Button _StartButton;

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

  void Start() {
    _ForceProgressSecretButton.onClick.AddListener(HandleForceProgressPressed);
    _StartButton.onClick.AddListener(HandleStartButton);
    _StartNoEdgeSecretButton.onClick.AddListener(HandleStartNoEdgeButton);
    RobotEngineManager.Instance.DisconnectedFromClient += OnClientDisconnect;
  }

  private void OnClientDisconnect(DisconnectionReason reason) {
    _FatalErrorText.text = "Disconnected from Cozmo: " + reason.ToString();
  }

  public void SetPressDemoDebugState(int index) {
    _PressDebugBaseImage.overrideSprite = _PressDebugDatabase[index];
  }

  public void HideStartButtons() {
    _StartButton.gameObject.SetActive(false);
    _StartNoEdgeSecretButton.gameObject.SetActive(false);
  }

  private void HandleStartButton() {
    if (OnStartButton != null) {
      OnStartButton(true);
    }
    RobotEngineManager.Instance.CurrentRobot.StartDemoWithEdge(true);
    HideStartButtons();
  }

  private void HandleStartNoEdgeButton() {
    if (OnStartButton != null) {
      OnStartButton(false);
    }
    RobotEngineManager.Instance.CurrentRobot.StartDemoWithEdge(false);
    HideStartButtons();
  }

  private void HandleForceProgressPressed() {
    if (OnForceProgress != null) {
      OnForceProgress();
    }
  }

  protected override void CleanUp() {
    RobotEngineManager.Instance.DisconnectedFromClient -= OnClientDisconnect;
  }
}

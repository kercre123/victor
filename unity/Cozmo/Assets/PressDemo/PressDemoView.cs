using UnityEngine;
using System.Collections;

public class PressDemoView : Cozmo.UI.BaseView {

  public System.Action OnForceProgress;
  public System.Action<bool> OnStartButton;

  [SerializeField]
  private UnityEngine.UI.Button _ForceProgressButton;

  [SerializeField]
  private UnityEngine.UI.Button _StartButton;

  [SerializeField]
  private UnityEngine.UI.Button _StartNoEdgeButton;

  [SerializeField]
  private UnityEngine.UI.Text _FatalErrorText;

  [SerializeField]
  private PressDemoDebugState _PressDemoDebugState;

  void Start() {
    _ForceProgressButton.onClick.AddListener(HandleForceProgressPressed);
    _StartButton.onClick.AddListener(HandleStartButton);
    _StartNoEdgeButton.onClick.AddListener(HandleStartNoEdgeButton);
    RobotEngineManager.Instance.DisconnectedFromClient += OnClientDisconnect;
  }

  private void OnClientDisconnect(DisconnectionReason reason) {
    _FatalErrorText.text = "Disconnected from Cozmo: " + reason.ToString();
  }

  public void SetPressDemoDebugState(int index) {
    _PressDemoDebugState.SetDebugImageIndex(index);
  }

  public void HideStartButtons() {
    _StartButton.gameObject.SetActive(false);
    _StartNoEdgeButton.gameObject.SetActive(false);
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

using UnityEngine;
using System.Collections;

public class PressDemoView : Cozmo.UI.BaseView {

  public System.Action OnForceProgress;

  [SerializeField]
  private UnityEngine.UI.Button _ForceProgressButton;

  [SerializeField]
  private UnityEngine.UI.Button _StartButton;

  [SerializeField]
  private UnityEngine.UI.Button _StartNoEdgeButton;

  [SerializeField]
  private UnityEngine.UI.Text _DebugState;

  void Start() {
    _ForceProgressButton.onClick.AddListener(HandleForceProgressPressed);
    _StartButton.onClick.AddListener(HandleStartButton);
    _StartNoEdgeButton.onClick.AddListener(HandleStartNoEdgeButton);
  }

  private void HideStartButtons() {
    _StartButton.gameObject.SetActive(false);
    _StartNoEdgeButton.gameObject.SetActive(false);
  }

  private void HandleStartButton() {
    RobotEngineManager.Instance.CurrentRobot.StartDemoWithEdge(true);
    HideStartButtons();
  }

  private void HandleStartNoEdgeButton() {
    RobotEngineManager.Instance.CurrentRobot.StartDemoWithEdge(false);
    HideStartButtons();
  }

  private void HandleForceProgressPressed() {
    if (OnForceProgress != null) {
      OnForceProgress();
    }
  }

  void Update() {
    _DebugState.text = RobotEngineManager.Instance.CurrentRobot.CurrentBehaviorString;
  }

  protected override void CleanUp() {
    
  }
}

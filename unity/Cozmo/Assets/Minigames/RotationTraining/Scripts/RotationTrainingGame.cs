using UnityEngine;
using System.Collections;

namespace RotationTraining {
  public class RotationTrainingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    [SerializeField]
    private RotationTrainingPanel _GamePanelPrefab;
    private RotationTrainingPanel _GamePanel;

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();

      // TODO: Create a countdown state so that the player has a few seconds to prep.

      initCubeState.InitialCubeRequirements(new RotateState(), 1, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      _GamePanel = UIManager.OpenDialog(_GamePanelPrefab).GetComponent<RotationTrainingPanel>();
      _GamePanel.SetTimeLeft(0);
      CreateDefaultQuitButton();
    }
	
    // Update is called once per frame
    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    void InitialCubesDone() {
      
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }

    public void SetTimeLeft(int secondsLeft) {
      _GamePanel.SetTimeLeft(secondsLeft);
    }
  }
}
using UnityEngine;
using System.Collections;

namespace FollowCube {

  public class FollowCubeGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    [SerializeField]
    private FollowCubeGamePanel gamePanelPrefab_;

    private FollowCubeGamePanel gamePanel_;

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeState(), 1, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      gamePanel_ = UIManager.OpenDialog(gamePanelPrefab_).GetComponent<FollowCubeGamePanel>();
      gamePanel_.OnSlowSpeedPressed += SetSlowSpeed;
      gamePanel_.OnFastSpeedPressed += SetFastSpeed;
      gamePanel_.OnDemonSpeedPressed += SetDemonSpeed;

      CreateDefaultQuitButton();
    }

    public override void CleanUp() {
      if (gamePanel_ != null) {
        UIManager.CloseDialogImmediately(gamePanel_);
      }

      DestroyDefaultQuitButton();
    }

    // Update is called once per frame
    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    void InitialCubesDone() {
      SetSlowSpeed();
    }

    void SetSlowSpeed() {
      ForwardSpeed = 25.0f;
      DistanceMax = 120.0f;
      DistanceMin = 80.0f;
    }

    void SetFastSpeed() {
      ForwardSpeed = 50.0f;
      DistanceMax = 130.0f;
      DistanceMin = 90.0f;
    }

    void SetDemonSpeed() {
      ForwardSpeed = 200.0f;
      DistanceMax = 200.0f;
      DistanceMin = 0.0f;
    }
  }

}

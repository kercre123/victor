using UnityEngine;
using System.Collections;

public class FollowCubeGame : GameBase {

  private StateMachineManager stateMachineManager_ = new StateMachineManager();
  private StateMachine stateMachine_ = new StateMachine();

  void Start() {
    stateMachine_.SetGameRef(this);
    stateMachineManager_.AddStateMachine("FollowCubeStateMachine", stateMachine_);
    InitialCubesState initCubeState = new InitialCubesState();
    initCubeState.InitialCubeRequirements(new FollowCubeState(), 1, InitialCubesDone);
    stateMachine_.SetNextState(initCubeState);
    robot.StopFaceAwareness();
  }
	
  // Update is called once per frame
  void Update() {
    stateMachineManager_.UpdateAllMachines();
  }

  void InitialCubesDone() {
    
  }
}

using UnityEngine;
using System.Collections;

public class TreasureHuntGame : GameBase {

  private StateMachineManager stateMachineManager_ = new StateMachineManager();
  private StateMachine stateMachine_ = new StateMachine();

  public Vector2 GoldPosition { get; set; }

  void Start() {
    stateMachine_.SetGameRef(this);
    stateMachineManager_.AddStateMachine("TreasureHuntStateMachine", stateMachine_);
    InitialCubesState initCubeState = new InitialCubesState();
    initCubeState.InitialCubeRequirements(new LookForCubesState(), 1, InitialCubesDone);
    stateMachine_.SetNextState(initCubeState);
    robot.StopFaceAwareness();
  }

  void InitialCubesDone() {
    PickNewGoldPosition();
  }

  // Robot was picked up and placed down so the World Origin likely changed
  // due to robot de-localization. We have to pick a new position for the gold.
  void RobotPutDown() {
    PickNewGoldPosition();
  }

  public void PickNewGoldPosition() {
    
  }

  void Update() {
    stateMachineManager_.UpdateAllMachines();
  }

  public override void CleanUp() {

  }
}

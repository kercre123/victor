using UnityEngine;
using System.Collections;
using System.Collections.Generic;

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

  public void ClearBlockLights() {
    foreach (KeyValuePair<int, LightCube> kvp in robot.LightCubes) {
      kvp.Value.SetLEDs(0, 0, 0, 0);
    }
  }

  public bool HoveringOverGold(LightCube cube) {
    Vector2 blockPosition = (Vector2)cube.WorldPosition;
    bool hovering = Vector2.Distance(blockPosition, GoldPosition) < 15.0f;

    if (hovering) {
      for (int i = 0; i < cube.Lights.Length; ++i) {
        cube.Lights[i].onColor = CozmoPalette.ColorToUInt(Color.yellow);
      }
    }
    else {
      // set directional lights

    }

    return hovering;
  }

  void Update() {
    stateMachineManager_.UpdateAllMachines();
  }

  public override void CleanUp() {

  }
}

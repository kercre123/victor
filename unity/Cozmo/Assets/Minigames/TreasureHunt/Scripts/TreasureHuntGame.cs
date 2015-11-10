using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class TreasureHuntGame : GameBase {

  private StateMachineManager _StateMachineManager = new StateMachineManager();
  private StateMachine _StateMachine = new StateMachine();

  public Vector2 GoldPosition { get; set; }

  void Start() {
    _StateMachine.SetGameRef(this);
    _StateMachineManager.AddStateMachine("TreasureHuntStateMachine", _StateMachine);
    InitialCubesState initCubeState = new InitialCubesState();
    initCubeState.InitialCubeRequirements(new LookForGoldCubeState(), 1, InitialCubesDone);
    _StateMachine.SetNextState(initCubeState);
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
    // pick a random position from robot's world origin.
    // this may be improved by looking at where the robot has
    // been, where it is, where it is facing etc.
    GoldPosition = Random.insideUnitCircle * 200.0f;
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
      Vector2 cubeToTarget = GoldPosition - (Vector2)cube.WorldPosition;
      Vector2 cubeForward = (Vector2)cube.Forward;

      cubeToTarget.Normalize();
      cubeForward.Normalize(); 

      cube.SetLEDs(0);

      float dotVal = Vector2.Dot(cubeToTarget, cubeForward);

      if (dotVal > 0.5f) {
        // front
        cube.Lights[0].onColor = CozmoPalette.ColorToUInt(Color.yellow);
      }
      else if (dotVal < -0.5f) {
        // back
        cube.Lights[2].onColor = CozmoPalette.ColorToUInt(Color.yellow);
      }
      else {
        float crossSign = cubeToTarget.x * cubeForward.y - cubeToTarget.y * cubeForward.x;
        if (crossSign < 0.0f) {
          // left
          cube.Lights[1].onColor = CozmoPalette.ColorToUInt(Color.yellow);
        }
        else {
          // right
          cube.Lights[3].onColor = CozmoPalette.ColorToUInt(Color.yellow);
        }
      }
    }

    return hovering;
  }

  void Update() {
    _StateMachineManager.UpdateAllMachines();
  }

  public override void CleanUp() {

  }
}

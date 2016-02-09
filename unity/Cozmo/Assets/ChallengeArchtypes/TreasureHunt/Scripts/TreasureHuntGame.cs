using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace TreasureHunt {

  public class TreasureHuntGame : GameBase {

    public Vector2 GoldPosition { get; set; }

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      // TODO
      InitializeMinigameObjects(minigameConfig.NumCubesRequired());
    }

    protected void InitializeMinigameObjects(int numCubes) {
      InitialCubesState initCubeState = new InitialCubesState(new HowToPlayState(new LookForGoldCubeState()), numCubes);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
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
      foreach (KeyValuePair<int, LightCube> kvp in CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(0, 0, 0, 0);
      }
    }

    public void SetDirectionalLight(LightCube cube, float distance) {

      // if distance is really small then let's not set lights at all.
      if (distance < 0.01f) {
        return;
      }
        
      // flash based on on close the object is to the target.
      int flashComp = (int)(Time.time * 200.0f / distance);
      if (flashComp % 2 == 0) {
        return;
      }

      // set directional lights
      Vector2 cubeToTarget = GoldPosition - (Vector2)cube.WorldPosition;
      Vector2 cubeForward = (Vector2)cube.Forward;

      cubeToTarget.Normalize();
      cubeForward.Normalize();

      cube.SetLEDs(0);

      float dotVal = Vector2.Dot(cubeToTarget, cubeForward);

      if (dotVal > 0.5f) {
        // front
        cube.Lights[0].OnColor = Color.yellow.ToUInt();
      }
      else if (dotVal < -0.5f) {
        // back
        cube.Lights[2].OnColor = Color.yellow.ToUInt();
      }
      else {
        float crossSign = cubeToTarget.x * cubeForward.y - cubeToTarget.y * cubeForward.x;
        if (crossSign < 0.0f) {
          // left
          cube.Lights[1].OnColor = Color.yellow.ToUInt();
        }
        else {
          // right
          cube.Lights[3].OnColor = Color.yellow.ToUInt();
        }
      }
    }

    public void SetHoveringLight(LightCube cube) {
      for (int i = 0; i < cube.Lights.Length; ++i) {
        cube.Lights[i].OnColor = Color.yellow.ToUInt();
      }
    }

    public bool HoveringOverGold(LightCube cube, out float distance) {
      Vector2 blockPosition = (Vector2)cube.WorldPosition;
      distance = Vector2.Distance(blockPosition, GoldPosition);
      return distance < 15.0f;
    }

    protected override void CleanUpOnDestroy() {
    }
  }

}

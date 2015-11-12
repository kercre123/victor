using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace VisionTraining {

  public class VisionTrainingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new RecognizeCubeState(), 3, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      robot.StopFaceAwareness();

      CreateDefaultQuitButton();
    }

    void Update() {

    }

    private void InitialCubesDone() {
      
    }

    public int PickCube() {
      int index = Random.Range(0, robot.LightCubes.Count);
      int i = 0;
      foreach (KeyValuePair<int, LightCube> lightCube in robot.LightCubes) {
        if (index == i) {
          return lightCube.Key;
        }
        i++;
      }
      return -1;
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }
  }

}

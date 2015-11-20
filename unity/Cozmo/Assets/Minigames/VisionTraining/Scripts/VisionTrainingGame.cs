using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace VisionTraining {

  public class VisionTrainingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private int _LastSelectedId = -1;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {
      // TODO
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new RecognizeCubeState(), 2, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      CreateDefaultQuitButton();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    private void InitialCubesDone() {
      
    }

    public int PickCube() {

      // for weird edge case of if batteries die and there's only one cube left.
      if (CurrentRobot.LightCubes.Count == 1) {
        foreach (KeyValuePair<int, LightCube> lightCube in CurrentRobot.LightCubes) {
          return lightCube.Key;
        }
      }

      int id = -2;

      do {
        int index = Random.Range(0, CurrentRobot.LightCubes.Count);
        int i = 0;

        foreach (KeyValuePair<int, LightCube> lightCube in CurrentRobot.LightCubes) {
          if (index == i) {
            id = lightCube.Key;
            lightCube.Value.SetLEDs(CozmoPalette.ColorToUInt(Color.blue));
          }
          else {
            lightCube.Value.SetLEDs(0);
          }
          i++;
        }
      } while (id == _LastSelectedId);

      _LastSelectedId = id;

      return id;
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }
  }

}

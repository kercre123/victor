using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Simon {

  public class SimonGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    // list of ids of LightCubes that are tapped, in order.
    private List<int> _CurrentIDSequence = new List<int>();

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      InitializeMinigameObjects();
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() { 
      DAS.Info(this, "Game Created");

      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("SimonGameStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new CozmoSetSimonState(), 2, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
    }

    private void InitialCubesDone() {
      // give cubes colors
      List<Color> colors = new List<Color>();
      colors.Add(Color.white);
      colors.Add(Color.blue);
      colors.Add(Color.red);
      colors.Add(Color.magenta);
      colors.Add(Color.green);
      foreach (KeyValuePair<int, LightCube> kvp in CurrentRobot.LightCubes) {
        int colorIndex = Random.Range(0, colors.Count);
        kvp.Value.SetLEDs(colors[colorIndex]);
        colors.RemoveAt(colorIndex);
        if (colors.Count == 0) {
          break;
        }
      }
    }

    public void PickNewSequence() {
      _CurrentIDSequence.Clear();
      int sequenceLength = Random.Range(3, 5);
      for (int i = 0; i < sequenceLength; ++i) {
        int pickedID = -1;
        int pickIndex = Random.Range(0, CurrentRobot.LightCubes.Count);
        int j = 0;
        foreach (KeyValuePair<int, LightCube> kvp in CurrentRobot.LightCubes) {
          if (j == pickIndex) {
            pickedID = kvp.Key;
            break;
          }
          ++j;
        }
        _CurrentIDSequence.Add(pickedID);
      }
    }

    public IList<int> GetCurrentSequence() {
      return _CurrentIDSequence.AsReadOnly();
    }

    protected override void CleanUpOnDestroy() {

    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

  }

}

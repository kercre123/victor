using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace Simon {

  public class SimonGame : GameBase {
    public const float kDriveWheelSpeed = 80f;
    public const float kDotThreshold = 0.96f;

    // list of ids of LightCubes that are tapped, in order.
    private List<int> _CurrentIDSequence = new List<int>();

    private SimonGameConfig _Config;

    public int MaxSequenceLength { get { return _Config.MaxSequenceLength; } }

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = (SimonGameConfig)minigameConfig;
      InitializeMinigameObjects();
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() { 
      DAS.Info(this, "Game Created");
      NumSegments = MaxSequenceLength;
      MaxAttempts = _Config.MaxAttempts;
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WaitForNextTurnState(_Config.MinSequenceLength), 2, true, null);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
    }

    public void PickNewSequence(int sequenceLength) {

      // give cubes colors
      List<Color> colors = new List<Color>();
      colors.Add(Color.white);
      colors.Add(Color.blue);
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

      _CurrentIDSequence.Clear();
      for (int i = 0; i < sequenceLength; ++i) {
        int pickedID = -1;
        int pickIndex = Random.Range(0, CurrentRobot.LightCubes.Count);
        pickedID = CurrentRobot.LightCubes.ElementAt(pickIndex).Key;
        _CurrentIDSequence.Add(pickedID);
      }
    }

    public IList<int> GetCurrentSequence() {
      return _CurrentIDSequence.AsReadOnly();
    }

    protected override void CleanUpOnDestroy() {

    }

  }

}

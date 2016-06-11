using UnityEngine;
using System.Collections.Generic;

namespace Simon {
  public class ScanForInitialCubeState : InitialCubesState {

    private enum ScannedSetupCubeState {
      Unknown,
      Seen,
      TooFar,
      TooClose,
      Ready
    };

    // TODO: move to real states if we're keeping this
    // Right now it's one of 3 possible setups
    private enum ScanState {
      NoCubesSeen,
      WaitForContinue,
      Scan,
      Stopped,
    }

    private Dictionary<int, ScannedSetupCubeState> _SetupCubeState;
    private bool _CubesStateUpdated;
    private Color _CubeTooFarColor;
    private Color _CubeTooCloseColor;
    private const string _kLookInPlaceForCubesBehaviorName = "simon_lookInPlaceForCubes";

    private ScanState _ScanState;

    private int _NumValidCubes;
    // TODO: Keep GameIDs sorted instead
    List<LightCube> _SortedCubes = new List<LightCube>();

    public ScanForInitialCubeState(State nextState, int cubesRequired, Color CubeTooFar, Color CubeTooClose) : base(nextState, cubesRequired) {
      _SetupCubeState = new Dictionary<int, ScannedSetupCubeState>();
      _CubesStateUpdated = false;
      _CubeTooFarColor = CubeTooFar;
      _CubeTooCloseColor = CubeTooClose;
      _ScanState = ScanState.NoCubesSeen;
      _NumValidCubes = 0;
      _SortedCubes = new List<LightCube>();
    }

    public override void Enter() {
      base.Enter();

      foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
        lightCube.Value.SetLEDsOff();
      }
    }

    // ignore base class events
    protected override void HandleInFieldOfViewStateChanged(ObservedObject changedObject, ObservedObject.InFieldOfViewState oldState,
                                                   ObservedObject.InFieldOfViewState newState) {
    }
    protected override void CheckForNewlySeenCubes() {
    }

    public override void Update() {
      // Intentionally avoid base class since that will only check currently visible cubes
      if (_ScanState == ScanState.NoCubesSeen) {
        if (_CurrentRobot.VisibleLightCubes.Count > 0) {
          UpdateScannedCubes();
          SetScanState(ScanState.WaitForContinue);
        }
      }
      else {
        UpdateScannedCubes();
        if (_Game.CubeIdsForGame.Count != _NumValidCubes || _CubesStateUpdated) {
          _NumValidCubes = _Game.CubeIdsForGame.Count;
          if (_NumValidCubes == _CubesRequired) {
            SetScanState(ScanState.Stopped);
          }
          _CubesStateUpdated = false;
          UpdateUI(_NumValidCubes);
        }
      }
    }

    private LightCube GetClosestAvailableBlock(LightCube other) {
      float minDist = float.MaxValue;
      ObservedObject closest = null;
      foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
        if (other != lightCube.Value) {
          float d = Vector3.Distance(lightCube.Value.WorldPosition, other.WorldPosition);
          if (d < minDist) {
            minDist = d;
            closest = lightCube.Value;
          }
        }
      }
      return closest as LightCube;
    }

    private void UpdateSetupLightState(int cubeID, ScannedSetupCubeState state) {
      if (state != _SetupCubeState[cubeID]) {
        _SetupCubeState[cubeID] = state;
        _CubesStateUpdated = true;
        LightCube cube = _CurrentRobot.LightCubes[cubeID];
        if (state == ScannedSetupCubeState.Seen) {
          cube.SetLEDs(Cozmo.CubePalette.InViewColor.lightColor);
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedBlockConnect);
        }
        else if (state == ScannedSetupCubeState.TooClose) {
          cube.SetLEDs(_CubeTooCloseColor);
        }
        else if (state == ScannedSetupCubeState.TooFar) {
          cube.SetLEDs(_CubeTooFarColor);
        }
        else if (state == ScannedSetupCubeState.Ready) {
          cube.SetLEDs(Cozmo.CubePalette.ReadyColor.lightColor);
        }
      }
    }

    private ScannedSetupCubeState UpdateSetupCubeState(LightCube cubeA, LightCube cubeB) {
      float minDistMM = 60.0f;
      float maxDistMM = 140.0f;
      float dist = Vector3.Distance(cubeA.WorldPosition, cubeB.WorldPosition);
      if (dist < minDistMM) {
        return ScannedSetupCubeState.TooClose;
      }
      else if (dist > maxDistMM) {
        return ScannedSetupCubeState.TooFar;
      }
      return ScannedSetupCubeState.Ready;
    }

    private void UpdateSetupCubeState(int checkIndex, List<LightCube> sortedCubes) {
      LightCube currCube = sortedCubes[checkIndex];
      if (sortedCubes.Count == 1) {
        UpdateSetupLightState(currCube.ID, ScannedSetupCubeState.Seen);
      }
      else if (checkIndex == 0) {
        // One to your right.
        UpdateSetupLightState(currCube.ID, UpdateSetupCubeState(currCube, sortedCubes[checkIndex + 1]));
      }
      else if (checkIndex == sortedCubes.Count - 1) {
        // One to your left
        UpdateSetupLightState(currCube.ID, UpdateSetupCubeState(currCube, sortedCubes[checkIndex - 1]));
      }
      else {
        // Check both right and left for cubes in the middle.
        ScannedSetupCubeState cubeLeftState = UpdateSetupCubeState(currCube, sortedCubes[checkIndex - 1]);
        ScannedSetupCubeState cubeRightState = UpdateSetupCubeState(currCube, sortedCubes[checkIndex + 1]);
        if (cubeLeftState != ScannedSetupCubeState.Ready) {
          UpdateSetupLightState(currCube.ID, cubeLeftState);
        }
        else if (cubeRightState != ScannedSetupCubeState.Ready) {
          UpdateSetupLightState(currCube.ID, cubeRightState);
        }
        else {
          UpdateSetupLightState(currCube.ID, ScannedSetupCubeState.Ready);
        }
      }
    }

    protected void UpdateScannedCubes() {
      LightCube cube = null;
      _SortedCubes.Clear();
      foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
        cube = lightCube.Value;

        // TODO: if clear if was known but is no longer known and clear from _ValidCubeIDs list.
        if (cube.MarkersVisible) {
          if (!_Game.CubeIdsForGame.Contains(cube.ID)) {
            if (_Game.CubeIdsForGame.Count < _CubesRequired) {
              _Game.CubeIdsForGame.Add(cube.ID);
              _SetupCubeState.Add(cube.ID, ScannedSetupCubeState.Unknown);
              cube.SetLEDs(Cozmo.CubePalette.InViewColor.lightColor);
            }
          }
        }

        if (_Game.CubeIdsForGame.Contains(cube.ID)) {
          _SortedCubes.Add(cube);
        }
      }

      // Theres only 3 cubes, so shouldn't take that long.
      _SortedCubes.Sort(new BlockToCozmoPositionComparer(_CurrentRobot));

      for (int i = 0; i < _SortedCubes.Count; ++i) {
        UpdateSetupCubeState(i, _SortedCubes);
      }

    }

    protected override void UpdateUI(int numValidCubes) {
      _ShowCozmoCubesSlide.LightUpCubes(_NumValidCubes);

      bool is_valid = false;
      if (_NumValidCubes >= _CubesRequired) {
        // Check the distance and number
        _Game.SharedMinigameView.SetContinueButtonSupplementText(GetCubesReadyText(_CubesRequired), Cozmo.UI.UIColorPalette.CompleteTextColor);
        int readyCubes = 0;
        foreach (KeyValuePair<int, ScannedSetupCubeState> cubeState in _SetupCubeState) {
          if (cubeState.Value == ScannedSetupCubeState.Ready) {
            readyCubes++;
          }
        }
        if (readyCubes >= _CubesRequired) {
          is_valid = true;
        }

      }

      if (!is_valid) {
        _Game.SharedMinigameView.SetContinueButtonSupplementText(GetWaitingForCubesText(_CubesRequired), Cozmo.UI.UIColorPalette.NeutralTextColor);
      }
      // TODO: when done prototyping clean this up
      if (_ScanState == ScanState.WaitForContinue) {
        _Game.SharedMinigameView.EnableContinueButton(true);
      }
      else {
        _Game.SharedMinigameView.EnableContinueButton(is_valid);
      }

    }

    protected override void HandleContinueButtonClicked() {
      if (_ScanState == ScanState.WaitForContinue) {
        SetScanState(ScanState.Scan);
      }
      else {
        base.HandleContinueButtonClicked();
      }
    }

    // TODO: prototype only, once settled on setup method, move.
    private void SetScanState(ScanState NextState) {
      if (_ScanState != NextState) {
        if (NextState == ScanState.WaitForContinue) {
          _Game.SharedMinigameView.EnableContinueButton(true);
        }
        if (NextState == ScanState.Scan) {
          // being in SetEnableFreeplayBehaviorChooser already forces us to be in the selection chooser.
          _CurrentRobot.ExecuteBehaviorByName(_kLookInPlaceForCubesBehaviorName);
          _Game.SharedMinigameView.EnableContinueButton(false);
        }
        else if (NextState == ScanState.Stopped) {
          _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
          _CurrentRobot.TurnTowardsObject(_SortedCubes[1], false);
        }
        _ScanState = NextState;
      }
    }
    public override void Exit() {
      base.Exit();
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
    }

  }
}
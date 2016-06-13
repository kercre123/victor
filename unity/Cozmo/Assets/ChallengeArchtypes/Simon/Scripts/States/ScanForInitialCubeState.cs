using UnityEngine;
using System.Collections.Generic;

namespace Simon {
  public class ScanForInitialCubeState : InitialCubesState {

    private enum ScannedSetupCubeState {
      Unknown,
      Seen,
      TooClose,
      Ready
    };

    private enum ScanPhase {
      NoCubesSeen,
      WaitForContinue,
      ScanLeft,
      ScanRight,
      ScanCenter,
      Stopped,
      Error,
    }

    private Dictionary<int, ScannedSetupCubeState> _SetupCubeState;
    private bool _CubesStateUpdated;
    private Color _CubeTooCloseColor;
    private const string _kLookInPlaceForCubesBehaviorName = "simon_lookInPlaceForCubes";

    private ScanPhase _ScanPhase;

    private int _NumValidCubes;
    // TODO: Keep GameIDs sorted instead
    List<LightCube> _SortedCubes = new List<LightCube>();

    public ScanForInitialCubeState(State nextState, int cubesRequired, Color CubeTooClose) : base(nextState, cubesRequired) {
      _SetupCubeState = new Dictionary<int, ScannedSetupCubeState>();
      _CubesStateUpdated = false;
      _CubeTooCloseColor = CubeTooClose;
      _ScanPhase = ScanPhase.NoCubesSeen;
      _NumValidCubes = 0;
      _SortedCubes = new List<LightCube>();
    }

    public override void Enter() {
      base.Enter();

      foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
        lightCube.Value.SetLEDsOff();
      }

      _ShowCozmoCubesSlide.SetLabelText(Localization.Get(LocalizationKeys.kSimonGameLabelPlaceCenter));
    }

    // ignore base class events
    protected override void HandleInFieldOfViewStateChanged(ObservedObject changedObject, ObservedObject.InFieldOfViewState oldState,
                                                   ObservedObject.InFieldOfViewState newState) {
    }
    protected override void CheckForNewlySeenCubes() {
    }

    public override void Update() {
      // Intentionally avoid base class since that will only check currently visible cubes
      if (_ScanPhase == ScanPhase.NoCubesSeen) {
        int visibleLightCount = _CurrentRobot.VisibleLightCubes.Count;
        if (visibleLightCount > 0) {
          UpdateScannedCubes();
          // If Cozmo can see all at once, we know it's an error, just tell them immediately.
          if (visibleLightCount == _CubesRequired) {
            SetScanPhase(ScanPhase.Error);
          }
          else {
            SetScanPhase(ScanPhase.WaitForContinue);
          }
        }
      }
      else if (_ScanPhase != ScanPhase.Error) {
        UpdateScannedCubes();
        if (_Game.CubeIdsForGame.Count != _NumValidCubes || _CubesStateUpdated) {
          _NumValidCubes = _Game.CubeIdsForGame.Count;
          if (_NumValidCubes == _CubesRequired) {
            int readyCubes = 0;
            int closeCubes = 0;
            foreach (KeyValuePair<int, ScannedSetupCubeState> cubeState in _SetupCubeState) {
              if (cubeState.Value == ScannedSetupCubeState.Ready) {
                readyCubes++;
              }
              else if (cubeState.Value == ScannedSetupCubeState.TooClose) {
                closeCubes++;
              }
            }
            if (readyCubes == _CubesRequired) {
              SetScanPhase(ScanPhase.Stopped);
            }
            else if (closeCubes != 0) {
              SetScanPhase(ScanPhase.Error);
            }
          }
          _CubesStateUpdated = false;
          UpdateUI(_NumValidCubes);
        }
      }
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
        else if (state == ScannedSetupCubeState.Ready) {
          cube.SetLEDs(Cozmo.CubePalette.ReadyColor.lightColor);
        }
      }
    }

    private ScannedSetupCubeState UpdateSetupCubeState(LightCube cubeA, LightCube cubeB) {
      float minDistMM = 60.0f;
      float dist = Vector3.Distance(cubeA.WorldPosition, cubeB.WorldPosition);
      if (dist < minDistMM) {
        return ScannedSetupCubeState.TooClose;
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
      if (_ShowCozmoCubesSlide != null) {

        switch (numValidCubes) {
        case 1:
          // Start with lighting up the center specifically.
          _ShowCozmoCubesSlide.LightUpCubes(new List<int> { 1 });
          break;
        case 2:
          // Start with lighting up the center specifically.
          _ShowCozmoCubesSlide.LightUpCubes(new List<int> { 1, 2 });
          break;
        default:
          _ShowCozmoCubesSlide.LightUpCubes(numValidCubes);
          break;
        }

        //_ShowCozmoCubesSlide.LightUpCubes(_NumValidCubes);
        if (_ScanPhase != ScanPhase.Stopped) {
          _Game.SharedMinigameView.SetContinueButtonSupplementText(GetWaitingForCubesText(_CubesRequired), Cozmo.UI.UIColorPalette.NeutralTextColor);
        }
        // TODO: when done prototyping clean this up and move to setting phase.
        _Game.SharedMinigameView.EnableContinueButton(_ScanPhase == ScanPhase.WaitForContinue || _ScanPhase == ScanPhase.Stopped);

      }
      else {
        _Game.SharedMinigameView.SetContinueButtonSupplementText("", Cozmo.UI.UIColorPalette.NeutralTextColor);
      }

    }

    protected override void HandleContinueButtonClicked() {
      if (_ScanPhase == ScanPhase.WaitForContinue) {
        SetScanPhase(ScanPhase.ScanLeft);
      }
      else if (_ScanPhase == ScanPhase.Error) {
        SetScanPhase(ScanPhase.NoCubesSeen);
      }
      else {
        base.HandleContinueButtonClicked();
      }
    }
    private void SetScanPhase(ScanPhase nextState) {
      if (_ScanPhase != nextState) {
        // clean up previous
        if (_ScanPhase == ScanPhase.Error) {
          _ShowCozmoCubesSlide = _Game.SharedMinigameView.ShowCozmoCubesSlide(_CubesRequired);
          // Reset for another scan since hopefully they moved them
          _Game.CubeIdsForGame.Clear();
          _SetupCubeState.Clear();
          // Reset to seen so we don't error out immediately again and scan for being too close...
          foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
            lightCube.Value.SetLEDsOff();
          }
        }

        // setup next state...
        if (nextState == ScanPhase.WaitForContinue) {
          _Game.SharedMinigameView.EnableContinueButton(true);
        }
        else if (nextState == ScanPhase.NoCubesSeen) {
          _Game.SharedMinigameView.EnableContinueButton(false);
        }
        else if (nextState == ScanPhase.ScanLeft || nextState == ScanPhase.ScanCenter) {
          _Game.SharedMinigameView.EnableContinueButton(false);
          _CurrentRobot.TurnInPlace(Mathf.Deg2Rad * 45.0f, SimonGame.kTurnSpeed_rps, SimonGame.kTurnAccel_rps2, HandleTurnFinished);
          _ShowCozmoCubesSlide.RotateCozmoImageTo(45.0f, 2.0f);
        }
        else if (nextState == ScanPhase.ScanRight) {
          _Game.SharedMinigameView.EnableContinueButton(false);
          // Half speed since going further...
          _CurrentRobot.TurnInPlace(Mathf.Deg2Rad * -90.0f, SimonGame.kTurnSpeed_rps / 2, SimonGame.kTurnAccel_rps2, HandleTurnFinished);
          _ShowCozmoCubesSlide.RotateCozmoImageTo(-90.0f, 2.0f);
        }
        else if (nextState == ScanPhase.Stopped) {
          _ShowCozmoCubesSlide.RotateCozmoImageTo(0.0f, 2.0f);
          _CurrentRobot.TurnTowardsObject(_SortedCubes[1], false);
        }
        else if (nextState == ScanPhase.Error) {
          _ShowCozmoCubesSlide = null;
          _Game.SharedMinigameView.EnableContinueButton(true);
          if (_SortedCubes.Count > 1) {
            _CurrentRobot.TurnTowardsObject(_SortedCubes[1], false);
          }
          SimonGame simonGame = _Game as SimonGame;
          _Game.SharedMinigameView.ShowWideGameStateSlide(
                                                     simonGame.SimonSetupErrorPrefab.gameObject, "simon_error_slide");
        }
        _ScanPhase = nextState;
      }
    }

    private void HandleTurnFinished(bool success) {
      // Stopped or Error could have taken over
      if (_ScanPhase == ScanPhase.ScanLeft) {
        SetScanPhase(ScanPhase.ScanRight);
      }
      else if (_ScanPhase == ScanPhase.ScanRight) {
        SetScanPhase(ScanPhase.ScanCenter);
      }
      else if (_ScanPhase == ScanPhase.ScanCenter) {
        SetScanPhase(ScanPhase.ScanLeft);
      }
    }

    protected override string GetWaitingForCubesText(int numCubes) {
      if (_ScanPhase == ScanPhase.NoCubesSeen || _ScanPhase == ScanPhase.WaitForContinue) {
        return Localization.Get(LocalizationKeys.kSimonGameLabelWaitingForCubesPlaceCenter);
      }
      return Localization.Get(LocalizationKeys.kSimonGameLabelWaitingForCubesScanning);
    }

    public override void Exit() {
      base.Exit();
    }

  }
}
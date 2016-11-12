using UnityEngine;
using System.Collections.Generic;

namespace MemoryMatch {
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
    private ScanPhase _ScanPhase;
    private BlockToCozmoPositionComparerByID _BlockPosComparer;

    private float _MinDistBetweenCubesMM = 60.0f;
    private float _RotateSecScan = 2.0f;
    private float _ScanTimeoutSec = -1.0f;
    private float _ScanTimeoutSecMax = 30.0f;

    public ScanForInitialCubeState(State nextState, int cubesRequired, float MinDistBetweenCubesMM, float RotateSecScan, float ScanTimeoutSecMax) : base(nextState, cubesRequired) {
      _SetupCubeState = new Dictionary<int, ScannedSetupCubeState>();
      _CubesStateUpdated = false;
      _BlockPosComparer = new BlockToCozmoPositionComparerByID(_CurrentRobot);
      _MinDistBetweenCubesMM = MinDistBetweenCubesMM;
      _RotateSecScan = RotateSecScan;
      _ScanTimeoutSecMax = ScanTimeoutSecMax;
    }

    public override void Enter() {
      base.Enter();

      foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
        lightCube.Value.SetLEDsOff();
      }

      SetScanPhase(ScanPhase.NoCubesSeen);
      InitShowCubesSlide();
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Game_Start);
    }

    // ignore base class events
    protected override void CheckForNewlySeenCubes() {
    }

    public override void Update() {
      if (_CurrentRobot == null) {
        return;
      }
      // Intentionally avoid base class since that will only check currently visible cubes
      if (_ScanPhase == ScanPhase.NoCubesSeen) {
        int visibleLightCount = _CurrentRobot.VisibleLightCubes.Count;
        if (visibleLightCount > 0) {
          UpdateScannedCubes();
          // If Cozmo can see all at once, we know it's too close
          int closeCubes = 0;
          if (visibleLightCount == _CubesRequired) {
            foreach (KeyValuePair<int, ScannedSetupCubeState> cubeState in _SetupCubeState) {
              if (cubeState.Value == ScannedSetupCubeState.TooClose) {
                closeCubes++;
              }
            }
          }
          if (closeCubes > 0) {
            SetScanPhase(ScanPhase.Error);
          }
          else {
            SetScanPhase(ScanPhase.WaitForContinue);
          }
        }
      }
      else if (_ScanPhase != ScanPhase.Error) {
        UpdateScannedCubes();
        if (_CubesStateUpdated) {
          if (_Game.CubeIdsForGame.Count == _CubesRequired) {
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
          UpdateUI(_Game.CubeIdsForGame.Count);
        }
      }
    }

    private void UpdateSetupLightState(int cubeID, ScannedSetupCubeState state) {
      if (state != _SetupCubeState[cubeID]) {
        _SetupCubeState[cubeID] = state;
        _CubesStateUpdated = true;
        LightCube cube = _CurrentRobot.LightCubes[cubeID];
        if (state == ScannedSetupCubeState.Seen) {
          cube.SetLEDs(Cozmo.UI.CubePalette.Instance.InViewColor.lightColor);
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Block_Connect);
        }
        else if (state == ScannedSetupCubeState.TooClose) {
          cube.SetLEDs(Cozmo.UI.CubePalette.Instance.ErrorColor.lightColor);
        }
        else if (state == ScannedSetupCubeState.Ready) {
          cube.SetLEDs(Cozmo.UI.CubePalette.Instance.InViewColor.lightColor);
        }
      }
    }

    private ScannedSetupCubeState GetCubeDistance(LightCube cubeA, LightCube cubeB) {
      float dist = Vector3.Distance(cubeA.WorldPosition, cubeB.WorldPosition);
      if (dist < _MinDistBetweenCubesMM) {
        return ScannedSetupCubeState.TooClose;
      }
      return ScannedSetupCubeState.Ready;
    }

    private void UpdateSetupCubeState(int checkIndex) {
      if (_Game.CubeIdsForGame.Count == 1) {
        UpdateSetupLightState(_Game.CubeIdsForGame[checkIndex], ScannedSetupCubeState.Seen);
      }
      else if (checkIndex == 0) {
        // One to your right.
        LightCube currCube = _CurrentRobot.LightCubes[_Game.CubeIdsForGame[checkIndex]];
        LightCube otherCube = _CurrentRobot.LightCubes[_Game.CubeIdsForGame[checkIndex + 1]];
        UpdateSetupLightState(currCube, GetCubeDistance(currCube, otherCube));
      }
      else if (checkIndex == _Game.CubeIdsForGame.Count - 1) {
        // One to your left
        LightCube currCube = _CurrentRobot.LightCubes[_Game.CubeIdsForGame[checkIndex]];
        LightCube otherCube = _CurrentRobot.LightCubes[_Game.CubeIdsForGame[checkIndex - 1]];
        UpdateSetupLightState(currCube.ID, GetCubeDistance(currCube, otherCube));
      }
      else {
        // Check both right and left for cubes in the middle.
        LightCube currCube = _CurrentRobot.LightCubes[_Game.CubeIdsForGame[checkIndex]];
        LightCube leftCube = _CurrentRobot.LightCubes[_Game.CubeIdsForGame[checkIndex - 1]];
        LightCube rightCube = _CurrentRobot.LightCubes[_Game.CubeIdsForGame[checkIndex + 1]];
        ScannedSetupCubeState cubeLeftState = GetCubeDistance(currCube, leftCube);
        ScannedSetupCubeState cubeRightState = GetCubeDistance(currCube, rightCube);
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
      foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
        cube = lightCube.Value;
        bool cubeAdded = _Game.CubeIdsForGame.Contains(cube.ID);
        // our list of scanned cubes
        if (cube.IsInFieldOfView && !cubeAdded) {
          if (_Game.CubeIdsForGame.Count < _CubesRequired) {
            _Game.CubeIdsForGame.Add(cube.ID);
            _SetupCubeState.Add(cube.ID, ScannedSetupCubeState.Unknown);
            cube.SetLEDs(Cozmo.UI.CubePalette.Instance.InViewColor.lightColor);
          }
        }
        else if (cube.CurrentPoseState != ObservedObject.PoseState.Known && cubeAdded) {
          _Game.CubeIdsForGame.Remove(cube.ID);
          _SetupCubeState.Remove(cube.ID);
          cube.SetLEDs(Cozmo.UI.CubePalette.Instance.OutOfViewColor.lightColor);
        }
      }

      // Theres only 3 cubes, so shouldn't take that long.
      // And they tend to shift slowly enough where they won't get real move messages.
      _Game.CubeIdsForGame.Sort(_BlockPosComparer);

      for (int i = 0; i < _Game.CubeIdsForGame.Count; ++i) {
        UpdateSetupCubeState(i);
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
      }
    }

    protected override void HandleContinueButtonClicked() {
      if (_ScanPhase == ScanPhase.WaitForContinue) {
        SetScanPhase(ScanPhase.ScanLeft);
        _ScanTimeoutSec = Time.time + _ScanTimeoutSecMax;
      }
      else if (_ScanPhase == ScanPhase.Error) {
        SetScanPhase(ScanPhase.NoCubesSeen);
      }
      else {
        base.HandleContinueButtonClicked();
      }
    }

    private void InitShowCubesSlide() {
      if (_ShowCozmoCubesSlide == null) {
        Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Window_Open);
        _ShowCozmoCubesSlide = _Game.SharedMinigameView.ShowCozmoCubesSlide(_CubesRequired);
      }
      _ShowCozmoCubesSlide.SetLabelText(Localization.Get(LocalizationKeys.kMemoryMatchGameLabelPlaceCenter));
      _ShowCozmoCubesSlide.SetCubeSpacing(100);
    }

    private void SetScanPhase(ScanPhase nextState) {
      if (_ScanPhase != nextState && _CurrentRobot != null &&
          _Game != null && _Game.SharedMinigameView != null) {
        // clean up previous
        if (_ScanPhase == ScanPhase.Error) {
          InitShowCubesSlide();
          // Reset for another scan since hopefully they moved them
          _Game.CubeIdsForGame.Clear();
          _SetupCubeState.Clear();
          foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
            lightCube.Value.SetLEDsOff();
          }
        }

        // setup next state...
        if (nextState == ScanPhase.NoCubesSeen) {
          _Game.CubeIdsForGame.Clear();
          _SetupCubeState.Clear();
          UpdateUI(0);
          _Game.SharedMinigameView.EnableContinueButton(false);
        }
        else if (nextState == ScanPhase.WaitForContinue) {
          _Game.SharedMinigameView.EnableContinueButton(true);
        }
        else if (nextState == ScanPhase.ScanLeft || nextState == ScanPhase.ScanCenter) {
          _Game.SharedMinigameView.EnableContinueButton(false);
          const float kLeftScanDeg = 30.0f;
          const float kLeftScanUIDeg = 18.0f;
          _CurrentRobot.TurnInPlace(Mathf.Deg2Rad * kLeftScanDeg, MemoryMatchGame.kTurnSpeed_rps, MemoryMatchGame.kTurnAccel_rps2, HandleTurnFinished);
          if (_ShowCozmoCubesSlide != null) {
            _ShowCozmoCubesSlide.RotateCozmoImageTo(kLeftScanUIDeg, _RotateSecScan);
          }
        }
        else if (nextState == ScanPhase.ScanRight) {
          _Game.SharedMinigameView.EnableContinueButton(false);
          // Half speed since going further
          const float kRightScanDeg = -60.0f;
          const float kRightScanUIDeg = -18.0f;
          _CurrentRobot.TurnInPlace(Mathf.Deg2Rad * kRightScanDeg, MemoryMatchGame.kTurnSpeed_rps * 2, MemoryMatchGame.kTurnAccel_rps2, HandleTurnFinished);
          // Half of the total Degrees cozmo rotates since these are absolute          
          if (_ShowCozmoCubesSlide != null) {
            _ShowCozmoCubesSlide.RotateCozmoImageTo(kRightScanUIDeg, _RotateSecScan);
          }
        }
        else if (nextState == ScanPhase.Stopped) {
          // Rotate towards center
          if (_ShowCozmoCubesSlide != null) {
            _ShowCozmoCubesSlide.RotateCozmoImageTo(0.0f, _RotateSecScan);
          }
          LightCube centerCube = (_Game as MemoryMatchGame).GetCubeBySortedIndex(1);
          if (centerCube != null) {
            _CurrentRobot.TurnTowardsObject(centerCube, false);
          }
          // Force an autocontinue now...
          base.HandleContinueButtonClicked();
          //_Game.SharedMinigameView.EnableContinueButton(true);
        }
        else if (nextState == ScanPhase.Error) {
          _ShowCozmoCubesSlide = null;
          _Game.SharedMinigameView.EnableContinueButton(true);
          LightCube centerCube = (_Game as MemoryMatchGame).GetCubeBySortedIndex(1);
          if (centerCube != null) {
            _CurrentRobot.TurnTowardsObject(centerCube, false);
          }
          // Error sound
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Tap_Red);
          MemoryMatchGame MemoryMatchGame = _Game as MemoryMatchGame;
          _Game.SharedMinigameView.ShowWideGameStateSlide(
                                                     MemoryMatchGame.MemoryMatchSetupErrorPrefab.gameObject, "MemoryMatch_error_slide");
        }

        _ScanPhase = nextState;
        _Game.SharedMinigameView.SetContinueButtonSupplementText(GetWaitingForCubesText(_CubesRequired), Cozmo.UI.UIColorPalette.NeutralTextColor);
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
        if (_ScanTimeoutSec > 0 && Time.time > _ScanTimeoutSec) {
          _ScanTimeoutSec = -1.0f;
          _ShowCozmoCubesSlide.RotateCozmoImageTo(0.0f, _RotateSecScan);
          SetScanPhase(ScanPhase.NoCubesSeen);
        }
        else {
          SetScanPhase(ScanPhase.ScanLeft);
        }
      }
    }

    protected override string GetWaitingForCubesText(int numCubes) {
      if (_ScanPhase == ScanPhase.NoCubesSeen) {
        return Localization.Get(LocalizationKeys.kMemoryMatchGameLabelWaitingForCubesPlaceCenter);
      }
      else if (_ScanPhase == ScanPhase.WaitForContinue) {
        return Localization.Get(LocalizationKeys.kMemoryMatchGameLabelCubeReady);
      }
      else if (_ScanPhase == ScanPhase.Error) {
        return "";
      }
      return Localization.Get(LocalizationKeys.kMemoryMatchGameLabelWaitingForCubesScanning);
    }


  }
}
using UnityEngine;
using Anki.Debug;
using System.Collections.Generic;
using System.Linq;
using Cozmo.Util;

namespace Simon {

  public class SimonGame : GameBase {
    public const float kCozmoLightBlinkDelaySeconds = 0.1f;
    public const float kLightBlinkLengthSeconds = 0.3f;

    public const float kTurnSpeed_rps = 100f;
    public const float kTurnAccel_rps2 = 100f;

    // list of ids of LightCubes that are tapped, in order.
    private List<int> _CurrentIDSequence = new List<int>();
    private Dictionary<int, SimonCube> _BlockIdToSound = new Dictionary<int, SimonCube>();

    private SimonGameConfig _Config;

    public int MaxSequenceLength { get { return _Config.MaxSequenceLength; } }

    public float TimeBetweenBeats { get { return _Config.TimeBetweenBeats; } }

    public float TimeWaitFirstBeat { get { return _Config.TimeWaitFirstBeat; } }

    private int _CurrentSequenceLength;

    private PlayerType _FirstPlayer = PlayerType.Human;

    public AnimationCurve CozmoWinPercentage { get { return _Config.CozmoGuessCubeCorrectPercentage; } }

    [SerializeField]
    private SimonCube[] _CubeColorsAndSounds;

    public MusicStateWrapper BetweenRoundsMusic;

    private static bool _sShowWrongCubeTap = false;
    private void HandleDebugShowWrongTapColor(System.Object setvar) {
      _sShowWrongCubeTap = !_sShowWrongCubeTap;
    }

    protected override void Initialize(MinigameConfigBase minigameConfigData) {
      _Config = (SimonGameConfig)minigameConfigData;
      BetweenRoundsMusic = _Config.BetweenRoundsMusic;
      InitializeMinigameObjects();

      DebugConsoleData.Instance.AddConsoleFunctionUnity("Simon Toggle Debug Show Wrong", "Minigames", HandleDebugShowWrongTapColor);
    }

    protected override void CleanUpOnDestroy() {
      DebugConsoleData.Instance.RemoveConsoleFunctionUnity("Simon Toggle Debug Show Wrong", "Minigames");
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() {
      _CurrentSequenceLength = _Config.MinSequenceLength - 1;

      State nextState = new CozmoMoveCloserToCubesState(new WaitForNextRoundSimonState(_FirstPlayer));
      InitialCubesState initCubeState = new ScanForInitialCubeState(nextState, _Config.NumCubesRequired(),
                                                                    _Config.CubeTooFarColor, _Config.CubeTooCloseColor);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(GetDefaultMusicState());
    }

    public int GetNewSequenceLength(PlayerType playerPickingSequence) {
      _CurrentSequenceLength = _CurrentSequenceLength >= MaxSequenceLength ? MaxSequenceLength : _CurrentSequenceLength + 1;
      return _CurrentSequenceLength;
    }

    public void InitColorsAndSounds() {
      // Adds it so it's always R Y B based on Cozmo's left to right
      if (_BlockIdToSound.Count() == 0) {
        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSimonSetupComplete);
        List<LightCube> listCubes = new List<LightCube>();
        foreach (int cube in CubeIdsForGame) {
          listCubes.Add(CurrentRobot.LightCubes[cube]);
        }
        listCubes.Sort(new BlockToCozmoPositionComparer(CurrentRobot));
        int minCount = Mathf.Min(_CubeColorsAndSounds.Length, listCubes.Count);
        for (int i = 0; i < minCount; ++i) {
          LightCube cube = listCubes[i];
          _BlockIdToSound.Add(cube.ID, _CubeColorsAndSounds[i]);
          // Now just for easyID, set the normal list to the sorted order
          CubeIdsForGame[i] = cube.ID;
        }

        CurrentRobot.TurnOffAllLights();
      }
    }

    public void SetCubeLightsDefaultOn() {
      foreach (int cubeId in CubeIdsForGame) {
        CurrentRobot.LightCubes[cubeId].SetLEDs(_BlockIdToSound[cubeId].cubeColor);
      }
    }

    public void SetCubeLightsGuessWrong(int correctCubeID, int wrongTapCubeID = -1) {

      if (!CurrentRobot.LightCubes.ContainsKey(correctCubeID)) {
        List<int> validIDs = CurrentRobot.LightCubes.Keys.ToList<int>();
        string str = "";
        for (int i = 0; i < validIDs.Count; ++i) {
          str += validIDs[i] + ",";
        }
        DAS.Error("Simon.CubeDisconnect", "Cube Lost! " + correctCubeID + " valid are: " + str);
      }
      foreach (int cubeId in CubeIdsForGame) {
        if (_sShowWrongCubeTap && cubeId == wrongTapCubeID) {
          CurrentRobot.LightCubes[wrongTapCubeID].SetFlashingLEDs(Color.magenta, 100, 100, 0);
        }
        else if (cubeId == correctCubeID) {
          CurrentRobot.LightCubes[correctCubeID].SetFlashingLEDs(_BlockIdToSound[correctCubeID].cubeColor, 100, 100, 0);
        }
        else {
          CurrentRobot.LightCubes[cubeId].SetLEDsOff();
        }
      }
    }

    public void SetCubeLightsGuessRight() {
      foreach (int cubeId in CubeIdsForGame) {
        if (CurrentRobot.LightCubes.ContainsKey(cubeId)) {
          CurrentRobot.LightCubes[cubeId].SetFlashingLEDs(_BlockIdToSound[cubeId].cubeColor, 100, 100, 0);
        }
        else {
          List<int> validIDs = CurrentRobot.LightCubes.Keys.ToList<int>();
          string str = "";
          for (int i = 0; i < validIDs.Count; ++i) {
            str += validIDs[i] + ",";
          }

          DAS.Error("Simon.CubeDisconnect", "Cube Lost! " + cubeId + " valid are: " + str);
        }
      }
    }

    public void GenerateNewSequence(int sequenceLength) {
      // First time is special
      if (sequenceLength <= _Config.MinSequenceLength) {
        List<int> shuffledAllIDs = new List<int>(CubeIdsForGame);
        sequenceLength = sequenceLength > CubeIdsForGame.Count ? CubeIdsForGame.Count : sequenceLength;
        shuffledAllIDs.Shuffle();
        for (int i = 0; i < sequenceLength; ++i) {
          _CurrentIDSequence.Add(shuffledAllIDs[i]);
        }
      }
      else {
        int pickIndex = Random.Range(0, CubeIdsForGame.Count);
        int pickedID = CubeIdsForGame[pickIndex];
        // Attempt to decrease chance of 3 in a row
        if (_CurrentIDSequence.Count > 2 && CubeIdsForGame.Count > 1) {
          if (pickedID == _CurrentIDSequence[_CurrentIDSequence.Count - 2] &&
              pickedID == _CurrentIDSequence[_CurrentIDSequence.Count - 1]) {
            List<int> validIDs = new List<int>(CubeIdsForGame);
            validIDs.RemoveAt(pickIndex);
            pickIndex = Random.Range(0, validIDs.Count);
            pickedID = validIDs[pickIndex];
          }
        }
        _CurrentIDSequence.Add(pickedID);
      }
    }

    public IList<int> GetCurrentSequence() {
      return _CurrentIDSequence.AsReadOnly();
    }

    public Color GetColorForBlock(int blockId) {
      Color lightColor = Color.white;
      SimonCube simonCube;
      if (_BlockIdToSound.TryGetValue(blockId, out simonCube)) {
        lightColor = simonCube.cubeColor;
      }
      return lightColor;
    }

    public Anki.Cozmo.Audio.AudioEventParameter GetAudioForBlock(int blockId) {
      Anki.Cozmo.Audio.AudioEventParameter audioEvent =
        Anki.Cozmo.Audio.AudioEventParameter.SFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
      SimonCube simonCube;
      if (_BlockIdToSound.TryGetValue(blockId, out simonCube)) {
        audioEvent = (Anki.Cozmo.Audio.AudioEventParameter)simonCube.soundName;
      }
      return audioEvent;
    }

    public void UpdateSequenceText(string locKey, int currentIndex, int sequenceCount) {
      string infoText = Localization.Get(locKey);
      infoText += Localization.kNewLine;
      infoText += Localization.GetWithArgs(LocalizationKeys.kSimonGameLabelStepsLeft, currentIndex, sequenceCount);
      SharedMinigameView.InfoTitleText = infoText;
    }

    protected override void RaiseMiniGameQuit() {
      base.RaiseMiniGameQuit();
      DAS.Event(DASConstants.Game.kQuitGameScore, _CurrentSequenceLength.ToString());
    }
  }

  [System.Serializable]
  public class SimonCube {
    // TODO: Store Anki.Cozmo.Audio.GameEvent.SFX instead of uint; apparently Unity
    // doesn't like that.
    public Anki.Cozmo.Audio.AudioEventParameter soundName;
    public Color cubeColor;
  }

  public enum PlayerType {
    Human,
    Cozmo
  }


}

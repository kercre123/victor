using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

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

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = (SimonGameConfig)minigameConfig;
      BetweenRoundsMusic = _Config.BetweenRoundsMusic;
      InitializeMinigameObjects();
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() { 
      _CurrentSequenceLength = _Config.MinSequenceLength - 1;

      State nextState = new CozmoMoveCloserToCubesState(new WaitForNextRoundSimonState(_FirstPlayer));
      InitialCubesState initCubeState = new InitialCubesState(nextState, _Config.NumCubesRequired());
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
        }

        SetCubeLightsDefaultOn();
      }
    }

    public void SetCubeLightsDefaultOn() {
      foreach (int cubeId in CubeIdsForGame) {
        CurrentRobot.LightCubes[cubeId].SetLEDs(_BlockIdToSound[cubeId].cubeColor);
      }
    }

    public void SetCubeLightsGuessWrong() {
      foreach (int cubeId in CubeIdsForGame) {
        CurrentRobot.LightCubes[cubeId].SetFlashingLEDs(Color.red, 100, 100, 0);
      }
    }

    public void SetCubeLightsGuessRight() {
      foreach (int cubeId in CubeIdsForGame) {
        CurrentRobot.LightCubes[cubeId].SetFlashingLEDs(_BlockIdToSound[cubeId].cubeColor, 100, 100, 0);
      }
    }

    public void GenerateNewSequence(int sequenceLength) {

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

    public IList<int> GetCurrentSequence() {
      return _CurrentIDSequence.AsReadOnly();
    }

    protected override void CleanUpOnDestroy() {

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

  // Sorts a list from left to right based on relative to Cozmo position
  public class BlockToCozmoPositionComparer : Comparer<LightCube> {
    IRobot CurrentRobot;

    public BlockToCozmoPositionComparer(IRobot robot) {
      CurrentRobot = robot;
    }

    public override int Compare(LightCube a, LightCube b) {
      Vector3 cozmoSpaceA = CurrentRobot.WorldToCozmo(a.WorldPosition);
      Vector3 cozmoSpaceB = CurrentRobot.WorldToCozmo(b.WorldPosition);
      if (cozmoSpaceA.y > cozmoSpaceB.y) {
        return 1;      
      }
      else if (cozmoSpaceA.y < cozmoSpaceB.y) {
        return -1;
      }
      return 0;
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

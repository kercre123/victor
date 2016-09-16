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
    private int _CurrLivesCozmo;
    private int _CurrLivesHuman;

    public int MinSequenceLength { get { return _Config.MinSequenceLength; } }

    public int MaxSequenceLength { get { return _Config.MaxSequenceLength; } }

    public int GetCurrentTurnNumber { get { return _CurrentSequenceLength - MinSequenceLength; } }

    public float TimeBetweenBeats { get { return _Config.TimeBetweenBeats; } }

    public float TimeWaitFirstBeat { get { return _Config.TimeWaitFirstBeat; } }

    private int _CurrentSequenceLength;

    private PlayerType _FirstPlayer = PlayerType.Cozmo;
    private int _FirstScore = 0;

    public AnimationCurve CozmoWinPercentage { get { return _Config.CozmoGuessCubeCorrectPercentage; } }

    [SerializeField]
    private SimonCube[] _CubeColorsAndSounds;

    [SerializeField]
    private SimonTurnSlide _SimonTurnSlidePrefab;
    private GameObject _SimonTurnSlide;
    [SerializeField]
    private float _BannerAnimationDurationSeconds = 1.5f;

    public SimonTurnSlide SimonTurnSlidePrefab {
      get { return _SimonTurnSlidePrefab; }
    }

    public PlayerType FirstPlayer {
      get { return _FirstPlayer; }
    }

    [SerializeField]
    private Transform _SimonSetupErrorPrefab;

    public Transform SimonSetupErrorPrefab {
      get { return _SimonSetupErrorPrefab; }
    }

    public enum SimonMode : int {
      VS = 0,
      SOLO = 1
    }

    ;

    protected override void InitializeGame(MinigameConfigBase minigameConfigData) {
      _Config = (SimonGameConfig)minigameConfigData;

      InitializeMinigameObjects();
    }

    protected override void CleanUpOnDestroy() {
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() {
      _CurrentSequenceLength = _Config.MinSequenceLength - 1;
      _CurrLivesCozmo = _Config.MaxLivesCozmo;
      _CurrLivesHuman = _Config.MaxLivesHuman;
      _ShowScoreboardOnComplete = false;

      State nextState = new SelectDifficultyState(new CozmoMoveCloserToCubesState(
                          new WaitForNextRoundSimonState()),
                          DifficultyOptions, HighestLevelCompleted());
      InitialCubesState initCubeState = new ScanForInitialCubeState(nextState, _Config.NumCubesRequired(),
                                          _Config.MinDistBetweenCubesMM, _Config.RotateSecScan, _Config.ScanTimeoutSec);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
    }

    protected override void OnDifficultySet(int difficulty) {
      if (difficulty == (int)SimonMode.SOLO) {
        _FirstPlayer = PlayerType.Human;
      }
    }

    public int GetNewSequenceLength(PlayerType playerPickingSequence) {
      _CurrentSequenceLength = _CurrentSequenceLength >= MaxSequenceLength ? MaxSequenceLength : _CurrentSequenceLength + 1;
      return _CurrentSequenceLength;
    }

    public void InitColorsAndSounds() {
      // Adds it so it's always R Y B based on Cozmo's right to left
      if (_BlockIdToSound.Count() == 0) {
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
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Lose);
      foreach (int cubeId in CubeIdsForGame) {
        if (cubeId == correctCubeID) {
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
      }
    }

    public void GenerateNewSequence(int sequenceLength) {
      // Fill in the min length or add one...
      do {
        int pickIndex = Random.Range(0, CubeIdsForGame.Count);
        int pickedID = CubeIdsForGame[pickIndex];
        // Attempt to decrease chance of 3 in a row
        if (_CurrentIDSequence.Count >= 2) {
          if (pickedID == _CurrentIDSequence[_CurrentIDSequence.Count - 2] &&
              pickedID == _CurrentIDSequence[_CurrentIDSequence.Count - 1]) {
            List<int> validIDs = new List<int>(CubeIdsForGame);
            validIDs.RemoveAt(pickIndex);
            pickIndex = Random.Range(0, validIDs.Count);
            pickedID = validIDs[pickIndex];
          }
        }
        _CurrentIDSequence.Add(pickedID);
      } while (_CurrentIDSequence.Count < _Config.MinSequenceLength);
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
        Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect);
      SimonCube simonCube;
      if (_BlockIdToSound.TryGetValue(blockId, out simonCube)) {
        audioEvent = simonCube.soundName;
      }
      return audioEvent;
    }

    public void ShowWinnerPicture(PlayerType player) {
      SimonTurnSlide simonTurnScript = GetSimonSlide();
      Sprite currentPortrait = SharedMinigameView.PlayerPortrait;
      string status = Localization.GetWithArgs(LocalizationKeys.kSimonGameTextPatternLength, Mathf.Max(_CurrentIDSequence.Count, _FirstScore));
      if (player == PlayerType.Cozmo) {
        currentPortrait = SharedMinigameView.CozmoPortrait;
      }
      simonTurnScript.ShowEndGame(currentPortrait, status);
    }

    public void FinalLifeComplete(PlayerType player) {
      // If in solo mode just quit out,
      // If in vs mode, if cozmo turn just switch to human turns, if human turn game over based on highest score...

      if (CurrentDifficulty == (int)SimonMode.SOLO) {
        PlayerRoundsWon = 1;
        CozmoRoundsWon = 0;
        ShowWinnerPicture(PlayerType.Human);
        ShowBanner(LocalizationKeys.kSimonGameLabelYouWin);
        StartBaseGameEnd(true);
      }
      else if (CurrentDifficulty == (int)SimonMode.VS) {
        if (player == _FirstPlayer) {
          // Kick off next turn and save score to compare...
          _FirstScore = _CurrentIDSequence.Count;
          _CurrentIDSequence.Clear();
          _StateMachine.SetNextState(new WaitForNextRoundSimonState(_FirstPlayer == PlayerType.Cozmo ? PlayerType.Human : PlayerType.Cozmo));
        }
        else {
          // compare who wins...
          if ((_FirstPlayer == PlayerType.Cozmo && _CurrentIDSequence.Count >= _FirstScore) ||
              (_FirstPlayer == PlayerType.Human && _CurrentIDSequence.Count < _FirstScore)) {
            ShowWinnerPicture(PlayerType.Human);
            ShowBanner(LocalizationKeys.kSimonGameLabelYouWin);
            PlayerRoundsWon = 1;
            CozmoRoundsWon = 0;
            StartBaseGameEnd(true);
          }
          else {
            ShowBanner(LocalizationKeys.kSimonGameLabelCozmoWin);
            ShowWinnerPicture(PlayerType.Cozmo);
            PlayerRoundsWon = 0;
            CozmoRoundsWon = 1;
            StartBaseGameEnd(false);
          }
        }
      }
    }

    private SimonTurnSlide GetSimonSlide() {
      if (_SimonTurnSlide == null) {
        _SimonTurnSlide = SharedMinigameView.ShowWideGameStateSlide(
          _SimonTurnSlidePrefab.gameObject, "simon_turn_slide");
      }
      return _SimonTurnSlide.GetComponent<SimonTurnSlide>();
    }
    public void StartFirstRoundMusic() {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Memory_Match_Full_Life);
    }

    public void ShowCurrentPlayerTurnStage(PlayerType player, bool isListening) {
      SimonTurnSlide simonTurnScript = GetSimonSlide();
      string statusLocKey = isListening ? LocalizationKeys.kSimonGameLabelListen : LocalizationKeys.kSimonGameLabelRepeat;
      if (player == PlayerType.Cozmo) {
        simonTurnScript.ShowCozmoLives(_CurrLivesCozmo, _Config.MaxLivesCozmo, LocalizationKeys.kSimonGameLabelCozmoTurn, statusLocKey);
      }
      else {
        Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Window_Open);
        simonTurnScript.ShowHumanLives(_CurrLivesHuman, _Config.MaxLivesHuman, LocalizationKeys.kSimonGameLabelYourTurn, statusLocKey);
      }

    }

    public void ShowCenterResult(bool enabled, bool correct = true) {
      SimonTurnSlide simonTurnScript = GetSimonSlide();
      simonTurnScript.ShowCenterImage(enabled, correct);
    }

    public int GetLivesRemaining(PlayerType player) {
      return player == PlayerType.Human ? _CurrLivesHuman : _CurrLivesCozmo;
    }

    public void DecrementLivesRemaining(PlayerType player) {
      if (player == PlayerType.Human) {
        _CurrLivesHuman--;
        if (_CurrLivesHuman <= 0) {
          Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Memory_Match_No_Lives);
        }
      }
      else {
        _CurrLivesCozmo--;
      }
      ShowCurrentPlayerTurnStage(player, false);
    }

    public void ShowBanner(string bannerKey) {
      string bannerText = Localization.Get(bannerKey);
      SharedMinigameView.PlayBannerAnimation(bannerText, null,
        _BannerAnimationDurationSeconds);
    }

    protected override void SendCustomEndGameDasEvents() {
      DAS.Event(DASConstants.Game.kQuitGameScore, _CurrentSequenceLength.ToString());
    }
  }

  [System.Serializable]
  public class SimonCube {
    public Anki.Cozmo.Audio.AudioEventParameter soundName;
    public Color cubeColor;
  }

  public enum PlayerType {
    None,
    Human,
    Cozmo
  }


}

using UnityEngine;
using Anki.Debug;
using System.Collections.Generic;
using System.Linq;
using Cozmo.Util;
using Anki.Cozmo;
using Anki.Cozmo.Audio;

namespace Simon {

  public class SimonGame : GameBase {
    public const float kLightBlinkLengthSeconds = 0.3f;

    public const float kTurnSpeed_rps = 100f;
    public const float kTurnAccel_rps2 = 100f;
    private const string kSkillCurve = "skillCurveIndex";

    // list of ids of LightCubes that are tapped, in order.
    private List<int> _CurrentIDSequence = new List<int>();
    private Dictionary<int, SimonCube> _BlockIdToSound = new Dictionary<int, SimonCube>();

    private SimonGameConfig _Config;
    private int _CurrLivesCozmo;
    private int _CurrLivesHuman;
    private AnimationCurve _SkillCurve = new AnimationCurve(new Keyframe(0, 1, 0, -0.06f), new Keyframe(5, 0.7f, -0.06f, 0));

    public int MinSequenceLength { get { return _Config.MinSequenceLength; } }

    public int MaxSequenceLength { get { return _Config.MaxSequenceLength; } }

    public int MinRoundBeforeLongReact { get { return _Config.MinRoundBeforeLongReact; } }

    public int GetCurrentTurnNumber { get { return _CurrentSequenceLength - MinSequenceLength; } }

    public float TimeBetweenBeats { get { return _Config.TimeBetweenBeat_Sec.Evaluate(_CurrentSequenceLength); } }

    public float TimeWaitFirstBeat { get { return _Config.TimeWaitFirstBeat; } }
    public SimonGameConfig Config { get { return _Config; } }

    private int _CurrentSequenceLength;

    private PlayerType _FirstPlayer = PlayerType.Human;
    private bool _WantsSequenceGrowth = true;

    public AnimationCurve CozmoWinPercentage { get { return _SkillCurve; } }

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
    public bool IsSoloMode() {
      return CurrentDifficulty == (int)SimonMode.SOLO;
    }

    public PlayerType CurrentPlayer { get; set; }

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

    protected override void AddDisabledReactionaryBehaviors() {
      base.AddDisabledReactionaryBehaviors();

      _DisabledReactionaryBehaviors.Add(BehaviorType.ReactToCliff);
      _DisabledReactionaryBehaviors.Add(BehaviorType.ReactToPickup);
      _DisabledReactionaryBehaviors.Add(BehaviorType.ReactToReturnedToTreads);
      _DisabledReactionaryBehaviors.Add(BehaviorType.ReactToUnexpectedMovement);
      _DisabledReactionaryBehaviors.Add(BehaviorType.ReactToCubeMoved);
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() {
      _CurrentSequenceLength = _Config.MinSequenceLength - 1;
      _CurrLivesCozmo = _Config.MaxLivesCozmo;
      _CurrLivesHuman = _Config.MaxLivesHuman;
      _ShowScoreboardOnComplete = false;
      _ShowEndWinnerSlide = true;
      CurrentPlayer = _FirstPlayer;

      State nextState = new SelectDifficultyState(new CozmoMoveCloserToCubesState(
                          new WaitForNextRoundSimonState()),
                          DifficultyOptions, HighestLevelCompleted());
      InitialCubesState initCubeState = new ScanForInitialCubeState(nextState, _Config.NumCubesRequired(),
                                          _Config.MinDistBetweenCubesMM, _Config.RotateSecScan, _Config.ScanTimeoutSec);
      _StateMachine.SetNextState(initCubeState);
      if (CurrentRobot != null) {
        CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
        CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
        CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      }
      int curveIndex = Mathf.Clamp((int)SkillSystem.Instance.GetSkillVal(kSkillCurve), 0, _Config.CozmoGuessCubeCorrectPercentagePerSkill.Length - 1);
      _SkillCurve = _Config.CozmoGuessCubeCorrectPercentagePerSkill[curveIndex];
    }

    public int GetNewSequenceLength(PlayerType playerPickingSequence, out bool SequenceGrown) {
      // Only increment on the first player
      SequenceGrown = false;
      if (playerPickingSequence == _FirstPlayer && _WantsSequenceGrowth) {
        SequenceGrown = true;
        _CurrentSequenceLength = _CurrentSequenceLength >= MaxSequenceLength ? MaxSequenceLength : _CurrentSequenceLength + 1;
      }
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

    private LightCube GetCubeByID(int cubeId) {
      if (CurrentRobot != null && CurrentRobot.LightCubes.ContainsKey(cubeId)) {
        return CurrentRobot.LightCubes[cubeId];
      }
      return null;
    }

    public void SetCubeLightsDefaultOn() {
      foreach (int cubeId in CubeIdsForGame) {
        LightCube cube = GetCubeByID(cubeId);
        if (cube != null) {
          cube.SetLEDs(_BlockIdToSound[cubeId].cubeColor);
        }
      }
    }

    public void SetCubeLightsGuessWrong(int correctCubeID, int wrongTapCubeID = -1) {
      foreach (int cubeId in CubeIdsForGame) {
        if (cubeId == correctCubeID) {
          LightCube cube = GetCubeByID(correctCubeID);
          if (cube != null) {
            cube.SetFlashingLEDs(_BlockIdToSound[correctCubeID].cubeColor, 100, 100, 0);
          }
        }
        else {
          LightCube cube = GetCubeByID(cubeId);
          if (cube != null) {
            cube.SetLEDsOff();
          }
        }
      }
    }

    public void SetCubeLightsGuessRight() {
      foreach (int cubeId in CubeIdsForGame) {
        LightCube cube = GetCubeByID(cubeId);
        if (cube != null) {
          cube.SetFlashingLEDs(_BlockIdToSound[cubeId].cubeColor, 100, 100, 0);
        }
      }
    }

    public void GenerateNewSequence(int sequenceLength) {
      // For the first 3, require them to be unique
      bool requireUnique = sequenceLength <= CubeIdsForGame.Count;
      while (_CurrentIDSequence.Count < sequenceLength) {
        int pickIndex = Random.Range(0, CubeIdsForGame.Count);
        int pickedID = CubeIdsForGame[pickIndex];
        if (requireUnique && _CurrentIDSequence.Contains(pickedID)) {
          List<int> validIDs = new List<int>(CubeIdsForGame);
          validIDs.RemoveAll(x => _CurrentIDSequence.Contains(x));
          if (validIDs.Count > 0) {
            pickIndex = Random.Range(0, validIDs.Count);
            pickedID = validIDs[pickIndex];
          }
        }
        else if (_CurrentIDSequence.Count >= 2) {
          // don't allow 3 in a row for rest of game
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
      GetSimonSlide().ShowStatusText(Localization.GetWithArgs(LocalizationKeys.kSimonGameTextPatternLength, _CurrentIDSequence.Count));
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
      AudioEventParameter audioEvent = AudioEventParameter.UIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect);
      SimonCube simonCube;
      if (_BlockIdToSound.TryGetValue(blockId, out simonCube)) {
        audioEvent = simonCube.soundName;
      }
      return audioEvent;
    }

    protected override void ShowWinnerState(EndState currentEndState, string overrideWinnerText = null, string footerText = "") {
      if (IsSoloMode()) {
        if (SaveHighScore()) {
          overrideWinnerText = Localization.Get(LocalizationKeys.kSimonGameSoloNewHighScore);
        }
        else {
          overrideWinnerText = Localization.Get(LocalizationKeys.kSimonGameSoloGameOver);
        }
      }
      base.ShowWinnerState(currentEndState, overrideWinnerText, Localization.GetWithArgs(LocalizationKeys.kSimonGameTextPatternLength, _CurrentIDSequence.Count));

      // Set Final Music State
      GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Memory_Match_Fanfare);
    }

    public void FinalLifeComplete() {

      UpdateUIForGameEnd();

      Anki.Cozmo.AnimationTrigger trigger = Anki.Cozmo.AnimationTrigger.Count;
      // Set the length as our score to make High Scores easier
      PlayerScore = _CurrentSequenceLength;
      if (CurrentDifficulty == (int)SimonMode.SOLO) {
        PlayerRoundsWon = 1;
        CozmoRoundsWon = 0;
        StartBaseGameEnd(true);
        trigger = Anki.Cozmo.AnimationTrigger.MemoryMatchSoloGameOver;
      }
      else if (CurrentDifficulty == (int)SimonMode.VS) {
        // compare who wins...
        if (_CurrLivesHuman > _CurrLivesCozmo) {
          PlayerRoundsWon = 1;
          CozmoRoundsWon = 0;
          StartBaseGameEnd(true);
          trigger = Anki.Cozmo.AnimationTrigger.MemoryMatchPlayerWinGame;
        }
        else if (_CurrLivesHuman < _CurrLivesCozmo) {
          PlayerRoundsWon = 0;
          CozmoRoundsWon = 1;
          StartBaseGameEnd(false);
          trigger = Anki.Cozmo.AnimationTrigger.MemoryMatchCozmoWinGame;
        }
        else {
          StartBaseGameEnd(EndState.Tie);
          trigger = Anki.Cozmo.AnimationTrigger.MemoryMatchPlayerWinGame;
        }
      }
      if (trigger != Anki.Cozmo.AnimationTrigger.Count && CurrentRobot != null) {
        CurrentRobot.SendAnimationTrigger(trigger);
      }
    }

    public override void StartBaseGameEnd(EndState endState) {
      base.StartBaseGameEnd(endState);
      if (CurrentDifficulty == (int)SimonMode.VS) {
        // So the skills system can ignore solo mode. Can be changed if events are classes or more filters
        GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnMemoryMatchVsComplete, _ChallengeData.ChallengeID, CurrentDifficulty, endState == EndState.PlayerWin, PlayerScore, CozmoScore, IsHighIntensityRound()));
      }
    }
    // event values to add on game completion
    protected override Dictionary<string, float> GetGameSpecificEventValues() {
      Dictionary<string, float> ret = new Dictionary<string, float>();
      ret.Add("human_lives", _CurrLivesHuman);
      return ret;
    }

    public LightCube GetCubeBySortedIndex(int index) {
      if (index < CubeIdsForGame.Count && CurrentRobot != null) {
        if (CurrentRobot.LightCubes.ContainsKey(CubeIdsForGame[index])) {
          return CurrentRobot.LightCubes[CubeIdsForGame[index]];
        }
      }
      return null;
    }

    public SimonTurnSlide GetSimonSlide() {
      if (_SimonTurnSlide == null) {
        _SimonTurnSlide = SharedMinigameView.ShowFullScreenGameStateSlide(
          _SimonTurnSlidePrefab.gameObject, "simon_turn_slide");
        SharedMinigameView.HideShelf();

        SimonTurnSlide turnUI = _SimonTurnSlide.GetComponent<SimonTurnSlide>();
        turnUI.ShowHumanLives(_CurrLivesHuman, _Config.MaxLivesHuman);
        if (CurrentDifficulty == (int)SimonMode.VS) {
          turnUI.ShowCozmoLives(_CurrLivesCozmo, _Config.MaxLivesCozmo);
        }
      }
      return _SimonTurnSlide.GetComponent<SimonTurnSlide>();
    }

    public void UpdateMusicRound() {
      // Using current lives left calculate current round
      // Max lives - lives left = [0, 2] then shift index + 1 for Rounds [1, 3]
      int round = _Config.MaxLivesHuman - _CurrLivesHuman + 1;
      GameAudioClient.SetMusicRoundState(round);
      GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Memory_Match);
    }

    public void ShowCurrentPlayerTurnStage(PlayerType player, bool isListening) {
      SimonTurnSlide simonTurnScript = GetSimonSlide();
      if (player == PlayerType.Cozmo) {
        simonTurnScript.ShowCozmoLives(_CurrLivesCozmo, _Config.MaxLivesCozmo);
        simonTurnScript.ShowCenterText("");
      }
      else {
        simonTurnScript.ShowHumanLives(_CurrLivesHuman, _Config.MaxLivesHuman);
        simonTurnScript.ShowCenterText(isListening ? "" : Localization.Get(LocalizationKeys.kSimonGameLabelRepeat));
      }
      CurrentPlayer = player;
    }

    public void ShowCenterResult(bool enabled, bool correct = true) {
      SimonTurnSlide simonTurnScript = GetSimonSlide();
      simonTurnScript.ShowCenterImage(enabled, correct);
      simonTurnScript.ShowCenterText("");
    }

    public int GetLivesRemaining(PlayerType player) {
      return player == PlayerType.Human ? _CurrLivesHuman : _CurrLivesCozmo;
    }

    public override void AddPoint(bool playerScored) {
      base.AddPoint(playerScored);
      if (CurrentPlayer == PlayerType.Human) {
        _WantsSequenceGrowth = playerScored;
      }
    }
    public void DecrementLivesRemaining(PlayerType player) {
      if (player == PlayerType.Human) {
        _CurrLivesHuman--;
        _WantsSequenceGrowth = false;
      }
      else {
        _CurrLivesCozmo--;
      }

      ShowCurrentPlayerTurnStage(player, false);
      ShowCenterResult(true, false);
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

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
    private Dictionary<int, SimonSound> _BlockIdToSound = new Dictionary<int, SimonSound>();

    private SimonGameConfig _Config;

    public int MaxSequenceLength { get { return _Config.MaxSequenceLength; } }

    private int _CurrentSequenceLength;

    private PlayerType _FirstPlayer = PlayerType.Cozmo;

    public AnimationCurve CozmoWinPercentage { get { return _Config.CozmoGuessCubeCorrectPercentage; } }

    [SerializeField]
    private SimonSound[] _CubeColorsAndSounds;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = (SimonGameConfig)minigameConfig;
      InitializeMinigameObjects();
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() { 
      DAS.Info(this, "Game Created");
      _CurrentSequenceLength = _Config.MinSequenceLength - 1;
      if (Random.Range(0f, 1f) > 0.5f) {
        _FirstPlayer = PlayerType.Human;
      }
      else {
        _FirstPlayer = PlayerType.Cozmo;
      }
      State nextState = new CozmoMoveCloserToCubesState(new WaitForNextRoundSimonState(_FirstPlayer));
      InitialCubesState initCubeState = new InitialCubesState(new HowToPlayState(nextState), _Config.NumCubesRequired());
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);
    }

    public int GetNewSequenceLength(PlayerType playerPickingSequence) {
      if (playerPickingSequence == _FirstPlayer) {
        _CurrentSequenceLength++;
        if (_CurrentSequenceLength > MaxSequenceLength) {
          _CurrentSequenceLength = MaxSequenceLength;
        }
      }
      return _CurrentSequenceLength;
    }

    public void InitColorsAndSounds() {
      if (_BlockIdToSound.Count() == 0) {
        List<int> usedIndices = new List<int>();
        int randomIndex = 0;
        SimonSound randomSimonSound;
        foreach (LightCube cube in CubesForGame) {
          do {
            randomIndex = Random.Range(0, _CubeColorsAndSounds.Length);
          } while (usedIndices.Contains(randomIndex));
          usedIndices.Add(randomIndex);
          randomSimonSound = _CubeColorsAndSounds[randomIndex];
          _BlockIdToSound.Add(cube.ID, randomSimonSound);
        }
      }

      foreach (KeyValuePair<int, LightCube> kvp in CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(_BlockIdToSound[kvp.Key].cubeColor);
      }
    }

    public void GenerateNewSequence(int sequenceLength) {
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

    public void SetCurrentSequence(List<int> newSequence) {
      _CurrentIDSequence = newSequence;
    }

    protected override void CleanUpOnDestroy() {

    }

    public Anki.Cozmo.Audio.AudioEventParameter GetAudioForBlock(int blockId) {
      Anki.Cozmo.Audio.AudioEventParameter audioEvent = 
        Anki.Cozmo.Audio.AudioEventParameter.SFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
      SimonSound sound;
      if (_BlockIdToSound.TryGetValue(blockId, out sound)) {
        audioEvent = (Anki.Cozmo.Audio.AudioEventParameter)sound.soundName;
      }
      return audioEvent;
    }

    protected override int CalculateExcitementStatRewards() {
      return Mathf.Max(0, _CurrentSequenceLength - 4);
    }

    public void UpdateSequenceText(string locKey, int currentIndex, int sequenceCount) {
      string infoText = Localization.Get(locKey);
      infoText += Localization.kNewLine;
      infoText += Localization.GetWithArgs(LocalizationKeys.kSimonGameLabelStepsLeft, currentIndex, sequenceCount);
      SharedMinigameView.ShowInfoTextSlideWithKey(infoText);
    }
  }

  [System.Serializable]
  public class SimonSound {
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

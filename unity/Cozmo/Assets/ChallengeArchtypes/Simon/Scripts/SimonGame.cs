using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace Simon {

  public class SimonGame : GameBase {

    public const float kLightBlinkLengthSeconds = 0.3f;

    // list of ids of LightCubes that are tapped, in order.
    private List<int> _CurrentIDSequence = new List<int>();
    private Dictionary<int, SimonSound> _BlockIdToSound = new Dictionary<int, SimonSound>();

    private SimonGameConfig _Config;

    public int MaxSequenceLength { get { return _Config.MaxSequenceLength; } }

    private int _CurrentSequenceLength;

    private PlayerType _FirstPlayer = PlayerType.Cozmo;

    public float CozmoWinPercentage { get { return _Config.CozmoGuessCubeCorrectPercentage; } }

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = (SimonGameConfig)minigameConfig;
      InitializeMinigameObjects();
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() { 
      DAS.Info(this, "Game Created");
      _CurrentSequenceLength = _Config.MinSequenceLength - 1;
      InitialCubesState initCubeState = new InitialCubesState();
      if (Random.Range(0f, 1f) > 0.5f) {
        _FirstPlayer = PlayerType.Human;
      }
      else {
        _FirstPlayer = PlayerType.Cozmo;
      }
      State nextState = new CozmoMoveCloserToCubesState(_FirstPlayer);
      initCubeState.InitialCubeRequirements(nextState, _Config.NumCubesRequired(), true, null);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MusicGroupStates.SILENCE);
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
      // give cubes colors
      List<Color> colors = new List<Color>();
      colors.Add(Color.green);
      colors.Add(Color.blue);
      colors.Add(Color.red);
      colors.Add(Color.yellow);
      colors.Add(Color.magenta);
      int colorCounter = 0;
      foreach (KeyValuePair<int, LightCube> kvp in CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(colors[colorCounter]);
        colorCounter++;
        colorCounter %= colors.Count;
      }

      _BlockIdToSound.Clear();
      int counter = 0;
      string cozmoAnimationName = "Simon_Cube";
      Anki.Cozmo.Audio.EventType[] playerAudio = { 
        Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_POSITIVE_02, 
        Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_POSITIVE_03,
        Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_POSITIVE_04
      };
      int smallestArrayLength = playerAudio.Length;
      SimonSound sound;
      foreach (var kvp in CurrentRobot.LightCubes) {
        sound = new SimonSound();
        sound.cozmoAnimationName = cozmoAnimationName;
        sound.playerSoundName = playerAudio[counter];
        _BlockIdToSound.Add(kvp.Key, sound);
        counter++;
        counter %= smallestArrayLength;
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

    public string GetCozmoAnimationForBlock(int blockId) {
      string animationName = AnimationName.kSeeOldPattern;
      SimonSound sound;
      if (_BlockIdToSound.TryGetValue(blockId, out sound)) {
        animationName = sound.cozmoAnimationName;
      }
      return animationName;
    }

    public Anki.Cozmo.Audio.EventType GetPlayerAudioForBlock(int blockId) {
      Anki.Cozmo.Audio.EventType audioEvent = Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_CLICK_GENERAL;
      SimonSound sound;
      if (_BlockIdToSound.TryGetValue(blockId, out sound)) {
        audioEvent = sound.playerSoundName;
      }
      return audioEvent;
    }

    protected override int CalculateExcitementStatRewards() {
      return Mathf.Max(0, _CurrentSequenceLength - 4);
    }
  }

  public class SimonSound {
    public string cozmoAnimationName;
    public Anki.Cozmo.Audio.EventType playerSoundName;
  }

  public enum PlayerType {
    Human,
    Cozmo
  }


}

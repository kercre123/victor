using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace Simon {

  public class SimonGame : GameBase {

    // list of ids of LightCubes that are tapped, in order.
    private List<int> _CurrentIDSequence = new List<int>();
    private Dictionary<int, SimonSound> _BlockIdToSound = new Dictionary<int, SimonSound>();

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
      initCubeState.InitialCubeRequirements(new WaitForNextCozmoRoundSimonState(_Config.MinSequenceLength), 2, true, null);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MusicGroupStates.SILENCE);
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

      _BlockIdToSound.Clear();
      int counter = 0;
      string[] cozmoAnimationNames = { "SimonSays_Cube00", "SimonSays_Cube01" };
      Anki.Cozmo.Audio.EventType[] playerAudio = { 
        Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_POSITIVE_02, 
        Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_POSITIVE_03
      };
      int smallestArrayLength = Mathf.Min(cozmoAnimationNames.Length, playerAudio.Length);
      SimonSound sound;
      foreach (var kvp in CurrentRobot.LightCubes) {
        sound = new SimonSound();
        sound.cozmoAnimationName = cozmoAnimationNames[counter];
        sound.playerSoundName = playerAudio[counter];
        _BlockIdToSound.Add(kvp.Key, sound);
        counter++;
        counter %= smallestArrayLength;
      }
    }

    public IList<int> GetCurrentSequence() {
      return _CurrentIDSequence.AsReadOnly();
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
  }

  public class SimonSound {
    public string cozmoAnimationName;
    public Anki.Cozmo.Audio.EventType playerSoundName;
  }
}

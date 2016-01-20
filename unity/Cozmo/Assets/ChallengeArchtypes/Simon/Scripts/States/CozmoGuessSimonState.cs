using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using AnimationGroups;

namespace Simon {
  public class CozmoGuessSimonState : State {

    private SimonGame _GameInstance;
    private IList<int> _CurrentSequence;
    private int _CurrentSequenceIndex;
    private bool? _ShouldWinGame;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.ShowHowToPlaySlide("WatchCozmoGuess");
      _CurrentSequence = _GameInstance.GetCurrentSequence();
      _CurrentSequenceIndex = -1;
      _ShouldWinGame = null;

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_POSITIVE_01);
    }

    public override void Update() {
      base.Update();
      if (_ShouldWinGame != null) {
        if (_ShouldWinGame == true) {
          CozmoWinGame();
        }
        else {
          CozmoLoseGame();
        }
      }

      _CurrentSequenceIndex++;
      if (_CurrentSequenceIndex >= _CurrentSequence.Count) {
        _ShouldWinGame = true;
      }
      else {
        // Determine if Cozmo wins on the last color of the sequence
        float coinFlip = Random.Range(0f, 1f);
        if (coinFlip > _GameInstance.CozmoWinPercentage) {
          _ShouldWinGame = false;
          int correctId = _CurrentSequence[_CurrentSequenceIndex];
          List<int> blockIds = new List<int>();
          foreach (int id in _CurrentRobot.LightCubes.Keys) {
            if (id != correctId) {
              blockIds.Add(id);
            }
          }
          int targetId = blockIds[Random.Range(0, blockIds.Count)];
          _StateMachine.PushSubState(new CozmoTurnToCubeSimonState(_CurrentRobot.LightCubes[targetId], true));
        }
        else {
          _StateMachine.PushSubState(new CozmoTurnToCubeSimonState(GetCurrentTarget(), true));
        }
      }
    }

    public LightCube GetCurrentTarget() {
      return _CurrentRobot.LightCubes[_CurrentSequence[_CurrentSequenceIndex]];
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(kvp.Value.Lights[0].OnColor, 0, uint.MaxValue, 0, 0, 0);
      }
    }

    private void CozmoLoseGame() {
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetFlashingLEDs(Color.red, 100, 100, 0);
      }

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MusicGroupStates.SILENCE);
      _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorFail, HandleOnCozmoLoseAnimationDone));
    }

    private void CozmoWinGame() {
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(kvp.Value.Lights[0].OnColor, 0, 100, 100, 0, 0);
      }

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MusicGroupStates.SILENCE);

      // TODO: Need to find a better animation than shocked; Cozmo should be determined to win 
      // and feel a bit thwarted 
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kWin, HandleOnCozmoWinAnimationDone));
    }

    private void HandleOnCozmoWinAnimationDone(bool success) {
      BlackoutLights();
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Cozmo));
    }

    private void HandleOnCozmoLoseAnimationDone(bool success) {
      BlackoutLights();
      _GameInstance.RaiseMiniGameWin();
    }

    private void BlackoutLights() {
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetFlashingLEDs(Color.black, uint.MaxValue, 0, 0);
      }
    }
  }
}

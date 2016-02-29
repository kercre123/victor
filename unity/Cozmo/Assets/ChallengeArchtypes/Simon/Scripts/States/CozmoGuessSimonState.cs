using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Simon {
  public class CozmoGuessSimonState : State {

    private SimonGame _GameInstance;
    private IList<int> _CurrentSequence;
    private int _CurrentSequenceIndex;
    private bool? _ShouldWinGame;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kSimonGameHeaderWatchCozmoGuess);
      _GameInstance.SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kSimonGameLabelWatchCozmoGuess);
      _GameInstance.SharedMinigameView.CozmoScoreboard.Dim = false;
      _GameInstance.SharedMinigameView.PlayerScoreboard.Dim = true;
      _CurrentSequence = _GameInstance.GetCurrentSequence();
      _CurrentSequenceIndex = -1;
      _ShouldWinGame = null;

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
    }

    public override void Update() {
      base.Update();
      if (_ShouldWinGame.HasValue) {
        if (_ShouldWinGame.GetValueOrDefault()) {
          CozmoWinGame();
        }
        else {
          CozmoLoseGame();
        }
      }
      else {
        _CurrentSequenceIndex++;
        if (_CurrentSequenceIndex >= _CurrentSequence.Count) {
          _ShouldWinGame = true;
        }
        else {
          float coinFlip = Random.Range(0f, 1f);
          if (coinFlip > _GameInstance.CozmoWinPercentage.Evaluate(_CurrentSequenceIndex)) {
            _ShouldWinGame = false;
            int correctId = _CurrentSequence[_CurrentSequenceIndex];
            List<int> blockIds = new List<int>();
            foreach (LightCube cube in _GameInstance.CubesForGame) {
              if (cube.ID != correctId) {
                blockIds.Add(cube.ID);
              }
            }
            int targetId = blockIds[Random.Range(0, blockIds.Count)];
            StartTurnToTarget(_CurrentRobot.LightCubes[targetId]);
          }
          else {
            StartTurnToTarget(GetCurrentTarget());
          }
        }
      }
    }

    private void StartTurnToTarget(LightCube target) {
      var cubeLightColor = target.Lights[0].OnColor;

      _CurrentRobot.SetAllBackpackBarLED(cubeLightColor);

      _StateMachine.PushSubState(new CozmoTurnToCubeSimonState(target));
    }

    public LightCube GetCurrentTarget() {
      return _CurrentRobot.LightCubes[_CurrentSequence[_CurrentSequenceIndex]];
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _GameInstance.SetCubeLightsDefaultOn();
    }

    private void CozmoLoseGame() {
      _GameInstance.SetCubeLightsGuessWrong();

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);
      _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorFail, HandleOnCozmoLoseAnimationDone));
    }

    private void CozmoWinGame() {
      _GameInstance.SetCubeLightsGuessRight();

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);

      // TODO: Need to find a better animation than shocked; Cozmo should be determined to win 
      // and feel a bit thwarted 
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kWin, HandleOnCozmoWinAnimationDone));
    }

    private void HandleOnCozmoWinAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Cozmo));
    }

    private void HandleOnCozmoLoseAnimationDone(bool success) {
      _GameInstance.RaiseMiniGameWin(Localization.GetWithArgs(
        LocalizationKeys.kSimonGameTextPatternLength, _CurrentSequence.Count));
    }
  }
}

using UnityEngine;
using System.Collections;

namespace Simon {
  public class WaitForNextRoundSimonState : State {

    private SimonGame _GameInstance;
    private PlayerType _NextPlayer;

    bool _CanAutoAdvance = false;

    public WaitForNextRoundSimonState(PlayerType nextPlayer = PlayerType.None, bool WantsAutoAdvance = false) {
      _NextPlayer = nextPlayer;
      _CanAutoAdvance = WantsAutoAdvance;
    }

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      bool isSoloMode = _GameInstance.CurrentDifficulty == (int)SimonGame.SimonMode.SOLO;
      // On first turn not known until entered...
      if (_NextPlayer == PlayerType.None) {
        _NextPlayer = _GameInstance.FirstPlayer;
        _GameInstance.StartFirstRoundMusic();
      }

      // This is the end if the second player goes and one of them is out of lives.
      // Like in Horse the first player must wait for second player
      // if they both run out then it is a tie.
      bool isGameOver = false;
      if (!isSoloMode && _NextPlayer == _GameInstance.FirstPlayer &&
          (_GameInstance.GetLivesRemaining(PlayerType.Human) <= 0 || _GameInstance.GetLivesRemaining(PlayerType.Cozmo) <= 0)) {
        _GameInstance.FinalLifeComplete();
        isGameOver = true;
      }
      if (isSoloMode && _GameInstance.GetLivesRemaining(PlayerType.Human) <= 0) {
        _GameInstance.FinalLifeComplete();
        isGameOver = true;
      }

      if (!isGameOver) {
        _GameInstance.ShowCurrentPlayerTurnStage(_NextPlayer, true);
        _GameInstance.SetCubeLightsDefaultOn();

        _CurrentRobot.TurnTowardsObject(_GameInstance.GetCubeBySortedIndex(1), false, SimonGame.kTurnSpeed_rps, SimonGame.kTurnAccel_rps2, HandleCozmoTurnComplete);

        if (_NextPlayer == PlayerType.Human) {
          if (_CanAutoAdvance) {
            _GameInstance.SharedMinigameView.PlayBannerAnimation(Localization.Get(LocalizationKeys.kSimonGameLabelListen), HandleAutoAdvance);
            Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Placeholder);
          }
          else {
            _GameInstance.GetSimonSlide().ShowPlayPatternButton(HandleContinuePressed);
          }
        }
      }
    }

    private void HandleCozmoTurnComplete(bool success) {
      if (_NextPlayer == PlayerType.Cozmo) {
        _StateMachine.SetNextState(new CozmoGuessSimonState());
      }
    }
    private void HandleAutoAdvance() {
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Placeholder);
      HandleContinuePressed();
    }

    private void HandleContinuePressed() {
      _GameInstance.GetSimonSlide().HidePlayPatternButton();
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(_GameInstance.GetDefaultMusicState());
      _StateMachine.SetNextState(new SetSequenceSimonState(_NextPlayer));
    }
  }
}
using UnityEngine;
using System.Collections;

namespace Simon {
  public class WaitForNextRoundSimonState : State {

    private SimonGame _GameInstance;
    private PlayerType _NextPlayer;

    public WaitForNextRoundSimonState(PlayerType nextPlayer) {
      _NextPlayer = nextPlayer;
    }

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.InitColorsAndSounds();

      _GameInstance.SharedMinigameView.ShowContinueButtonCentered(HandleContinuePressed,
        Localization.Get(LocalizationKeys.kButtonContinue));
      _GameInstance.SharedMinigameView.EnableContinueButton(true);

      string headerTextKey = (_NextPlayer == PlayerType.Human) ? 
        LocalizationKeys.kSimonGameLabelYourTurn : LocalizationKeys.kSimonGameLabelCozmoTurn;
      _GameInstance.SharedMinigameView.InfoTitleText = null;
      _GameInstance.SharedMinigameView.ShowInfoTextSlideWithKey(headerTextKey);

      _GameInstance.SharedMinigameView.CozmoScoreboard.Dim = (_NextPlayer != PlayerType.Cozmo);
      _GameInstance.SharedMinigameView.PlayerScoreboard.Dim = (_NextPlayer != PlayerType.Human);
    }

    private void HandleContinuePressed() {
      _GameInstance.SharedMinigameView.HideContinueButtonShelf();
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MUSIC.PLAYFUL);
      if (_NextPlayer == PlayerType.Cozmo) {
        _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, HandleOnCozmoStartAnimationDone));
      }
      else { // _NextPlayer == PlayerType.Human
        _StateMachine.SetNextState(new PlayerSetSequenceSimonState());
      }
    }

    private void HandleOnCozmoStartAnimationDone(bool success) {
      _StateMachine.SetNextState(new CozmoSetSequenceSimonState());
    }
  }
}
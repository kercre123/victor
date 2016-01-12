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
      SimonGameNextRoundPanel nextRoundPanel = _GameInstance.ShowHowToPlaySlide("NextRound").GetComponent<SimonGameNextRoundPanel>();
      nextRoundPanel.EnableContinueButton(true);
      nextRoundPanel.OnContinueButtonPressed += HandleContinuePressed;
      nextRoundPanel.SetNextPlayerText(_NextPlayer);
    }

    private void HandleContinuePressed() {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MusicGroupStates.PLAYFUL);
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
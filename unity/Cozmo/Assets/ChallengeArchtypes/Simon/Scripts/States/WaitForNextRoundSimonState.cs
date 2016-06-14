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
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(_GameInstance.BetweenRoundsMusic);

      _GameInstance.SharedMinigameView.ShowContinueButtonCentered(HandleContinuePressed,
        Localization.Get(LocalizationKeys.kButtonContinue), "next_round_of_play_continue_button");

      _GameInstance.OnTurnStage(_NextPlayer, true);

      _GameInstance.SetCubeLightsDefaultOn();

      _CurrentRobot.TurnTowardsObject(_CurrentRobot.LightCubes[_GameInstance.CubeIdsForGame[1]], false, SimonGame.kTurnSpeed_rps, SimonGame.kTurnAccel_rps2);
    }

    private void HandleContinuePressed() {
      _GameInstance.SharedMinigameView.HideContinueButton();
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(_GameInstance.GetDefaultMusicState());
      _StateMachine.SetNextState(new SetSequenceSimonState(_NextPlayer));
    }
  }
}
using UnityEngine;
using System.Collections;

namespace Simon {
  public class WaitForNextRoundSimonState : State {

    private SimonGame _GameInstance;
    private PlayerType _NextPlayer;

    private const float kDelay = 2f;
    private float _AutoAdvanceTimestamp;

    public WaitForNextRoundSimonState(PlayerType nextPlayer = PlayerType.None) {
      _NextPlayer = nextPlayer;

      _AutoAdvanceTimestamp = Time.time;
    }

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      // On first turn not known until entered...
      if (_NextPlayer == PlayerType.None) {
        _NextPlayer = _GameInstance.FirstPlayer;
      }
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(_GameInstance.BetweenRoundsMusic);

      _GameInstance.SharedMinigameView.ShowContinueButtonCentered(HandleContinuePressed,
        Localization.Get(LocalizationKeys.kButtonContinue), "next_round_of_play_continue_button");

      _GameInstance.ShowCurrentPlayerTurnStage(_NextPlayer, true);

      _GameInstance.SetCubeLightsDefaultOn();

      _CurrentRobot.TurnTowardsObject(_CurrentRobot.LightCubes[_GameInstance.CubeIdsForGame[1]], false, SimonGame.kTurnSpeed_rps, SimonGame.kTurnAccel_rps2);
    }

    public override void Update() {
      base.Update();
      if (Time.time - _AutoAdvanceTimestamp > kDelay) {
        HandleContinuePressed();
      }
    }

    private void HandleContinuePressed() {
      _GameInstance.SharedMinigameView.HideContinueButton();
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(_GameInstance.GetDefaultMusicState());
      _StateMachine.SetNextState(new SetSequenceSimonState(_NextPlayer));
    }
  }
}
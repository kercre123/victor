using UnityEngine;
using System.Collections;

namespace Simon {
  public class WaitForNextRoundSimonState : State {

    private SimonGame _GameInstance;
    private PlayerType _NextPlayer;

    private const float kDelay = 2f;
    private float _AutoAdvanceTimestamp;
    bool _CanAutoAdvance = false;

    public WaitForNextRoundSimonState(PlayerType nextPlayer = PlayerType.None) {
      _NextPlayer = nextPlayer;

      _AutoAdvanceTimestamp = Time.time;
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
        _CurrentRobot.TurnTowardsObject(_CurrentRobot.LightCubes[_GameInstance.CubeIdsForGame[1]], false, SimonGame.kTurnSpeed_rps, SimonGame.kTurnAccel_rps2);

        if (_NextPlayer == PlayerType.Human) {
          _GameInstance.SharedMinigameView.ShowContinueButtonCentered(HandleContinuePressed,
            Localization.Get(LocalizationKeys.kButtonContinue), "next_round_of_play_continue_button");
        }
        else {
          _StateMachine.SetNextState(new CozmoGuessSimonState());
        }
      }
    }
    public override void Update() {
      base.Update();
      if (_CanAutoAdvance && Time.time - _AutoAdvanceTimestamp > kDelay) {
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
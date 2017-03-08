using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using Cozmo.UI;

namespace SpeedTap {
  public class SpeedTapCubeSelectionState : State {
    private SpeedTapGame _SpeedTapGame;
    private int _PlayerIndexSelecting;
    private float _WantsContinueTimeStamp = -1.0f;

    public override void Enter() {
      base.Enter();

      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _PlayerIndexSelecting = -1;

      _SpeedTapGame.SharedMinigameView.ShowShelf();

      _SpeedTapGame.SharedMinigameView.HideMiddleBackground();
      _SpeedTapGame.SharedMinigameView.ShowSpinnerWidget();
      _SpeedTapGame.SharedMinigameView.HideInstructionsVideoButton();

      // Cozmo is a special case of needing a cube selected first, always the first one he sees.
      SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)_SpeedTapGame.GetFirstPlayerByType(PlayerType.Cozmo);
      if (playerInfo != null) {
        if (playerInfo.CubeID == -1 && _SpeedTapGame.CubeIdsForGame.Count > 0) {
          playerInfo.CubeID = _SpeedTapGame.CubeIdsForGame[0];
        }
      }

      SelectCubeForNextPlayer();
    }

    public override void Exit() {
      base.Exit();
      if (_SpeedTapGame != null) {
        _SpeedTapGame.SharedMinigameView.HideMiddleBackground();

        // Clear lights on all cubes
        foreach (var kvp in _CurrentRobot.LightCubes) {
          _SpeedTapGame.SetLEDs(kvp.Value.ID, Color.black);
        }
      }
    }

    public override void Update() {
      base.Update();

      if (_WantsContinueTimeStamp > 0 && _WantsContinueTimeStamp < Time.time) {
        _WantsContinueTimeStamp = -1.0f;
        MoveToNextState();
      }
    }

    private bool IsCubeClaimed(int cubeID) {
      int playerCount = _SpeedTapGame.GetPlayerCount();
      for (int i = 0; i < playerCount; ++i) {
        SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)_SpeedTapGame.GetPlayerByIndex(i);
        if (playerInfo.CubeID == cubeID) {
          return true;
        }
      }
      return false;

    }

    private void SelectCubeForNextPlayer() {
      _PlayerIndexSelecting++;
      // Selected everyone...
      if (_PlayerIndexSelecting >= _SpeedTapGame.GetPlayerCount()) {
        // if first round, and 3 player we want to do a 5 second hold
        if (_SpeedTapGame.GetPlayerCount() >= 3) {
          _SpeedTapGame.ShowPlayerTapConfirmSlide(-1);
          _WantsContinueTimeStamp = Time.time + _SpeedTapGame.GameConfig.MPTimeSetupHoldSec;
        }
        else {
          MoveToNextState();
        }
      }
      else {
        SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)_SpeedTapGame.GetPlayerByIndex(_PlayerIndexSelecting);
        playerInfo.playerRole = PlayerRole.Player;
        // Show currect UI for each user type
        if (playerInfo.playerType == PlayerType.Cozmo) {
          _SpeedTapGame.ShowWaitForCozmoSlide();
        }
        // Don't ever show the mid screen game in MP in hopes the more explicit
        // round start screen will de-confuse players about who they are the second round.
        else if (playerInfo.CubeID == -1 || _SpeedTapGame.GetPlayerCount() > 2) {
          _SpeedTapGame.ShowPlayerTapConfirmSlide(_PlayerIndexSelecting);
        }
        else {
          _SpeedTapGame.ShowPlayerTapNewRoundSlide();
        }

        // Light up all available cubes since they need to pick one
        // else Light up only their reserved cube
        if (playerInfo.CubeID != -1) {
          _SpeedTapGame.StartCycleCube(playerInfo.CubeID,
                     CubePalette.Instance.TapMeColor.lightColors,
                     CubePalette.Instance.TapMeColor.cycleIntervalSeconds);
        }
        else {
          foreach (var kvp in _CurrentRobot.LightCubes) {
            if (!IsCubeClaimed(kvp.Value.ID)) {
              _SpeedTapGame.StartCycleCube(kvp.Value.ID,
                CubePalette.Instance.TapMeColor.lightColors,
                CubePalette.Instance.TapMeColor.cycleIntervalSeconds);
            }
          }
        }

        // GameState will have to end for playergoal for humans due to validity checks
        playerInfo.SetGoal(SpeedTapPlayerGoalBaseSelectCube.Factory(playerInfo, playerInfo.CubeID,
                                                                    HandleCubeSelectionEnded, HandleCubeSelectedAttempted));
      }
    }

    private void HandleCubeSelectionEnded(PlayerInfo playerBase, int completedCode) {
      DAS.Debug("HandleCubeSelectionEnded", "completedCode " + completedCode);
      SpeedTapPlayerInfo player = (SpeedTapPlayerInfo)playerBase;
      if (player.playerType == PlayerType.Cozmo) {
        if (completedCode != PlayerGoalBase.SUCCESS) {
          _SpeedTapGame.ShowInterruptionQuitGameView("speed_tap_cube_lost_alert",
                                                             LocalizationKeys.kMinigameLostTrackOfBlockTitle,
                                                             LocalizationKeys.kMinigameLostTrackOfBlockDescription);
        }
        else {
          LightCubeForPlayer(player);
          SelectCubeForNextPlayer();
        }
      }

    }

    private void LightCubeForPlayer(SpeedTapPlayerInfo player) {
      // If in MP it needs to be their color, not others...
      if (_SpeedTapGame.GetPlayerCount() >= 3) {
        _SpeedTapGame.SetLEDs(player.CubeID, player.ScoreboardColor);
      }
      else {
        _SpeedTapGame.SetLEDs(player.CubeID, CubePalette.Instance.ReadyColor.lightColor);
      }
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Cube_Cozmo_Tap);
    }

    private void HandleCubeSelectedAttempted(PlayerInfo playerBase, int cubeID) {
      // Human behavior needs to be validated. so that they are tapping their own cube or an unclaimed cube
      // If successful we kill the goal and consider it successful
      bool valid = false;
      SpeedTapPlayerInfo player = (SpeedTapPlayerInfo)playerBase;
      if (player.playerType == PlayerType.Cozmo) {
        valid = true;
        _SpeedTapGame.SetCozmoOrigPos();
        _SpeedTapGame.SharedMinigameView.HideSpinnerWidget();
        // Cozmo's goal ends once the animation is done
      }
      else {
        if (cubeID == player.CubeID || (player.CubeID == -1 && !IsCubeClaimed(cubeID))) {
          valid = true;
          player.CubeID = cubeID;
          player.SetGoal(null);
        }
      }

      if (valid) {
        if (player.playerType != PlayerType.Cozmo) {
          LightCubeForPlayer(player);
          SelectCubeForNextPlayer();
        }
      }
    }

    private void MoveToNextState() {
      if (_SpeedTapGame != null) {
        _SpeedTapGame.SharedMinigameView.HideGameStateSlide();
        _StateMachine.SetNextState(new SpeedTapBeginRound());
      }
    }
  }
}
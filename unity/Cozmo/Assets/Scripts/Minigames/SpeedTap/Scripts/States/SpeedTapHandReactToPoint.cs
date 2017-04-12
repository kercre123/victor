using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapHandReactToPoint : State {

    private SpeedTapGame _SpeedTapGame;

    private bool _WasMistakeMade;

    private SpeedTapPlayerInfo _FirstTapper;

    private List<LightCube> _WinningCubes = new List<LightCube>();

    private bool _EndRobotAnimFinished = false;
    private bool _EndCubeAnimFinished = false;
    private bool _EndStateTriggered = false;

    private bool _IsPlayingWinGameAnimation = false;

    public SpeedTapHandReactToPoint(SpeedTapPlayerInfo firstTapper, bool mistakeMade) {
      _FirstTapper = firstTapper;
      _WasMistakeMade = mistakeMade;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      List<PlayerInfo> pointPlayers = new List<PlayerInfo>();
      if (!_WasMistakeMade) {
        pointPlayers.Add(_FirstTapper);
      }
      else {
        int playerCount = _SpeedTapGame.GetPlayerCount();
        for (int i = 0; i < playerCount; ++i) {
          PlayerInfo playerInfo = _SpeedTapGame.GetPlayerByIndex(i);
          if (playerInfo != _FirstTapper) {
            pointPlayers.Add(playerInfo);
          }
        }
      }

      _SpeedTapGame.AddPoint(pointPlayers, _WasMistakeMade);
      // Debug mode for Sudden Death
#if ANKI_DEV_CHEATS
      if (_SpeedTapGame.GetPlayerCount() > 2) {
        // Always in the order of cozmo,player,player... debug so whatever.
        if (SpeedTapGame.sWantsSuddenDeathHumanCozmo) {
          PlayerInfo playerInfo = _SpeedTapGame.GetPlayerByIndex(0);
          playerInfo.playerScoreRound = _SpeedTapGame.MaxScorePerRound;
          playerInfo = _SpeedTapGame.GetPlayerByIndex(1);
          playerInfo.playerScoreRound = _SpeedTapGame.MaxScorePerRound;
          playerInfo = _SpeedTapGame.GetPlayerByIndex(2);
          playerInfo.playerScoreRound = 1;
          SpeedTapGame.sWantsSuddenDeathHumanCozmo = false;
        }
        if (SpeedTapGame.sWantsSuddenDeathHumanHuman) {
          PlayerInfo playerInfo = _SpeedTapGame.GetPlayerByIndex(0);
          playerInfo.playerScoreRound = 1;
          playerInfo = _SpeedTapGame.GetPlayerByIndex(1);
          playerInfo.playerScoreRound = _SpeedTapGame.MaxScorePerRound;
          playerInfo = _SpeedTapGame.GetPlayerByIndex(2);
          playerInfo.playerScoreRound = _SpeedTapGame.MaxScorePerRound;
          SpeedTapGame.sWantsSuddenDeathHumanHuman = false;
        }
      }
#endif

      // Depends on points being scored first
      _SpeedTapGame.UpdateUI();

      if (_SpeedTapGame.IsRoundComplete()) {
        GameAudioClient.SetMusicState(_SpeedTapGame.BetweenRoundsMusic);
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Round_End);

        _SpeedTapGame.EndCurrentRound();
        // Hide Current Round in between rounds
        _SpeedTapGame.SharedMinigameView.InfoTitleText = string.Empty;

        if (_SpeedTapGame.IsGameComplete()) {
          UpdateBlockLights(_WasMistakeMade);
          _SpeedTapGame.UpdateUIForGameEnd();
          PlayReactToGameAnimationAndSendEvent();
        }
        else {
          UpdateBlockLights(_WasMistakeMade);
          _StateMachine.SetNextState(new SpeedTapReactToRoundEnd(_SpeedTapGame.GetPlayerMostPointsWon()));
        }
      }
      else {
        UpdateBlockLights(_WasMistakeMade);
        PlayReactToHandAnimation();
      }
    }

    public override void Pause(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
      // COZMO-2033; some of the win game animations cause Cozmo's cliff sensor to trigger
      // So in those cases don't show the "Cozmo Moved; Quit Game" dialog
      if (!_IsPlayingWinGameAnimation) {
        base.Pause(reason, reactionaryBehavior);
      }
    }

    public LightCube GetBlockByID(int id) {
      LightCube cube = null;
      if (_CurrentRobot != null) {
        _CurrentRobot.LightCubes.TryGetValue(id, out cube);
      }
      return cube;
    }
    private void UpdateBlockLights(bool wasMistakeMade) {
      if (_SpeedTapGame == null || _CurrentRobot == null) {
        return;
      }
      List<LightCube> losingBlocks = new List<LightCube>();

      int playerCount = _SpeedTapGame.GetPlayerCount();
      if (!wasMistakeMade) {
        _WinningCubes.Add(GetBlockByID(_FirstTapper.CubeID));
        for (int i = 0; i < playerCount; ++i) {
          SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)_SpeedTapGame.GetPlayerByIndex(i);
          if (playerInfo != _FirstTapper) {
            losingBlocks.Add(GetBlockByID(playerInfo.CubeID));
          }
        }
      }
      else {
        losingBlocks.Add(GetBlockByID(_FirstTapper.CubeID));
        for (int i = 0; i < playerCount; ++i) {
          SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)_SpeedTapGame.GetPlayerByIndex(i);
          if (playerInfo != _FirstTapper) {
            _WinningCubes.Add(GetBlockByID(playerInfo.CubeID));
          }
        }
      }

      if (wasMistakeMade) {
        for (int i = 0; i < _WinningCubes.Count; ++i) {
          if (_WinningCubes[i] != null) {
            _WinningCubes[i].SetLEDsOff();
          }
        }
        for (int i = 0; i < losingBlocks.Count; ++i) {
          if (losingBlocks[i] != null) {
            _CurrentRobot.PlayCubeAnimationTrigger(losingBlocks[i], CubeAnimationTrigger.SpeedTapLose,
                                                    (success) => { _EndCubeAnimFinished = true; HandleHandEndAnimDone(success); });
          }

        }
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_St_Lose);
      }
      else {
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_St_Win);
        for (int i = 0; i < losingBlocks.Count; ++i) {
          if (losingBlocks[i] != null) {
            losingBlocks[i].SetLEDsOff();
          }
        }
        for (int i = 0; i < _WinningCubes.Count; ++i) {
          if (_WinningCubes[i] != null) {
            _CurrentRobot.PlayCubeAnimationTrigger(_WinningCubes[i], CubeAnimationTrigger.SpeedTapWin,
                                            (success) => { _EndCubeAnimFinished = true; HandleHandEndAnimDone(success); });
          }
        }
      }
    }

    // This will return true if cozmo got a point that hand.
    // Which in MP he might not have won the game if that extra point was from a different players mistake
    private bool IsCozmoHandWinner() {
      bool cozmoWon = true;

      if (_WasMistakeMade && _FirstTapper.playerType == PlayerType.Cozmo) {
        cozmoWon = false;
      }
      else if (!_WasMistakeMade && _FirstTapper.playerType != PlayerType.Cozmo) {
        cozmoWon = false;
      }
      return cozmoWon;
    }

    private void PlayReactToHandAnimation() {
      AnimationTrigger animationEventToSend = IsCozmoHandWinner() ? AnimationTrigger.OnSpeedtapHandCozmoWin : AnimationTrigger.OnSpeedtapHandPlayerWin;
      ListenForAnimationEnd(animationEventToSend, (success) => { _EndRobotAnimFinished = true; HandleHandEndAnimDone(success); });
    }

    private void PlayReactToGameAnimationAndSendEvent() {
      // IsCozmoHandWinner just returns if he got a point that hand, this checks the whole game.
      // to cover the case where the final point can be given to two people and brings one person to game point.
      PlayerInfo winnerInfo = _SpeedTapGame.GetPlayerMostPointsWon();
      bool cozmoWonGame = winnerInfo.playerType == PlayerType.Cozmo;
      AnimationTrigger animationEventToSend = AnimationTrigger.Count;
      bool highIntensity = _SpeedTapGame.IsHighIntensityGame();
      if (DebugMenuManager.Instance.DemoMode) {
        animationEventToSend = !cozmoWonGame ?
                            AnimationTrigger.DemoSpeedTapCozmoLose : AnimationTrigger.DemoSpeedTapCozmoWin;
      }
      else {
        if (!cozmoWonGame) {
          animationEventToSend = (highIntensity) ?
                    AnimationTrigger.OnSpeedtapGamePlayerWinHighIntensity : AnimationTrigger.OnSpeedtapGamePlayerWinLowIntensity;
        }
        else {
          animationEventToSend = (highIntensity) ?
                    AnimationTrigger.OnSpeedtapGameCozmoWinHighIntensity : AnimationTrigger.OnSpeedtapGameCozmoWinLowIntensity;
        }
      }
      _IsPlayingWinGameAnimation = true;
      ListenForAnimationEnd(animationEventToSend, HandleEndGameAnimDone);
    }

    private void ListenForAnimationEnd(AnimationTrigger animEvent, RobotCallback animationCallback) {
      _CurrentRobot.SendAnimationTrigger(animEvent, animationCallback);
    }

    private void HandleHandEndAnimDone(bool success) {

      // If we are flashing loser lights on a cube then immediately transition states when the EndAnim completes
      // Otherwise only transition states if we have flashed the cube lights the desired number of times
      if (_EndCubeAnimFinished && _EndRobotAnimFinished && !_EndStateTriggered) {
        _EndStateTriggered = true;
        _SpeedTapGame.ClearWinningLightPatterns();
        _StateMachine.SetNextState(new SpeedTapHandCubesOff());
      }
    }

    private void HandleEndGameAnimDone(bool success) {
      GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Game_End);
      _SpeedTapGame.ClearWinningLightPatterns();
      _SpeedTapGame.StartRoundBasedGameEnd();
      _SpeedTapGame.StartEndMusic();
    }
  }
}

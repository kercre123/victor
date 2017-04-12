using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapHandCubesOn : State {

    private SpeedTapGame _SpeedTapGame;

    private float _LightsOnDuration_sec;
    private float _StartTimestamp_sec;

    private bool _ShouldMatch = false;
    private float _EndTimestamp_sec = -1;

    public SpeedTapHandCubesOn(bool shouldMatch) {
      _ShouldMatch = shouldMatch;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.ResetTapTimestamps();

      _StartTimestamp_sec = Time.time;
      _LightsOnDuration_sec = _SpeedTapGame.GetLightsOnDurationSec();
      float CozmoMovementDelay_sec = (_LightsOnDuration_sec * Random.Range(_SpeedTapGame.MinTapDelay_percent, _SpeedTapGame.MaxTapDelay_percent));

      // Set lights on cubes
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_St_Lightup);
      _SpeedTapGame.Rules.SetLights(shouldMatch: _ShouldMatch, game: _SpeedTapGame);

      int playerCount = _SpeedTapGame.GetPlayerCount();
      for (int i = 0; i < playerCount; ++i) {
        SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)_SpeedTapGame.GetPlayerByIndex(i);
        // During sudden death mode they become spectators...
        if (playerInfo.playerRole == PlayerRole.Player) {
          if (playerInfo.playerType == PlayerType.Human) {
            playerInfo.SetGoal(new SpeedTapPlayerGoalTapHuman());
          }
          else if (playerInfo.playerType == PlayerType.Cozmo) {
            float mistakeChance = _SpeedTapGame.CozmoMistakeChance;
            // dont mistake at all if in MP and it's "match point"
            if (playerCount > 2) {
              for (int j = 0; j < playerCount; ++j) {
                if (_SpeedTapGame.GetPlayerByIndex(j).playerScoreRound >= (_SpeedTapGame.MaxScorePerRound - 1)) {
                  mistakeChance = 0.0f;
                  break;
                }
              }
            }
            playerInfo.SetGoal(new SpeedTapPlayerGoalTapCozmo(CozmoMovementDelay_sec, _ShouldMatch,
                                    mistakeChance, _SpeedTapGame.CozmoFakeoutChance));
          }
          else if (playerInfo.playerType == PlayerType.Automation) {
            playerInfo.SetGoal(new SpeedTapPlayerGoalTapAuto(_ShouldMatch));
          }
        }
      }
    }

    private SpeedTapPlayerInfo GetFirstTapper() {
      float firstTapTime = float.MaxValue;
      SpeedTapPlayerInfo firstPlayer = null;
      int playerCount = _SpeedTapGame.GetPlayerCount();
      for (int i = 0; i < playerCount; ++i) {
        SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)_SpeedTapGame.GetPlayerByIndex(i);
        //  Make sure not in spectator mode during Sudden Death
        if (playerInfo.playerRole == PlayerRole.Player &&
            playerInfo.LastTapTimeStamp > 0 && playerInfo.LastTapTimeStamp < firstTapTime) {
          firstTapTime = playerInfo.LastTapTimeStamp;
          firstPlayer = playerInfo;
        }
      }
      return firstPlayer;
    }

    public override void Update() {
      base.Update();
      float secondsElapsed = Time.time - _StartTimestamp_sec;
      SpeedTapPlayerInfo firstTapper = GetFirstTapper();
      if (_EndTimestamp_sec < 0 && (firstTapper != null || secondsElapsed > _LightsOnDuration_sec)) {
        _EndTimestamp_sec = Time.time;
      }
      else if (_EndTimestamp_sec >= 0 && (Time.time - _EndTimestamp_sec > _SpeedTapGame.TapResolutionDelay)) {
        ResolveHand(firstTapper);
      }
    }

    public override void Exit() {
      base.Exit();
      // Players no longer need to set tap times.
      int playerCount = _SpeedTapGame.GetPlayerCount();
      for (int i = 0; i < playerCount; ++i) {
        SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)_SpeedTapGame.GetPlayerByIndex(i);
        playerInfo.SetGoal(null);
      }
    }

    /// <summary>
    /// Resolves the hand, first tapper loses, if no taps, we keep going.
    /// </summary>
    private void ResolveHand(SpeedTapPlayerInfo firstTapper = null) {
      // if no one tapped, just reset regardless
      if (firstTapper == null) {
        _StateMachine.SetNextState(new SpeedTapHandCubesOff());
      }
      else {
        _StateMachine.SetNextState(new SpeedTapHandReactToPoint(firstTapper, !_ShouldMatch));
      }
    }
  }
}
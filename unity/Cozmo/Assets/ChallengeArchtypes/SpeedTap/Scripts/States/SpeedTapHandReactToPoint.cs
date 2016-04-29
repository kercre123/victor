using UnityEngine;
using System.Collections;
using Anki.Cozmo.Audio;

namespace SpeedTap {
  public enum PointWinner {
    Cozmo,
    Player
  }

  public class SpeedTapHandReactToPoint : State {

    private const float _kWinCycleSpeed = 0.1f;

    private SpeedTapGame _SpeedTapGame;

    private PointWinner _CurrentWinner;
    private bool _WasMistakeMade;

    public SpeedTapHandReactToPoint(PointWinner winner, bool mistakeMade) {
      _CurrentWinner = winner;
      _WasMistakeMade = mistakeMade;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      Color[] winColors;
      LightCube winningBlock;
      LightCube losingBlock;
      if (_CurrentWinner == PointWinner.Player) {
        _SpeedTapGame.AddPlayerPoint();
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapWin);
        winColors = _SpeedTapGame.PlayerWinColors;
        winningBlock = _SpeedTapGame.PlayerBlock;
        losingBlock = _SpeedTapGame.CozmoBlock;
      }
      else {
        _SpeedTapGame.AddCozmoPoint();
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLose);
        winColors = _SpeedTapGame.CozmoWinColors;
        winningBlock = _SpeedTapGame.CozmoBlock;
        losingBlock = _SpeedTapGame.PlayerBlock;
      }

      _SpeedTapGame.UpdateUI();

      SetWinningLightPattern(winningBlock, winColors);
      SetLosingLightPattern(losingBlock, _WasMistakeMade);

      if (_SpeedTapGame.IsRoundComplete()) {
        HandleRoundEnd();
      }
      else {
        ReactToHand();
      }
    }

    public override void Exit() {
      base.Exit();
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapHandWin, HandleHandEndAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapHandLose, HandleHandEndAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapRoundWin, HandleRoundEndAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapRoundLose, HandleRoundEndAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapSessionWin, HandleSessionAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapSessionLose, HandleSessionAnimDone);
    }

    private void SetWinningLightPattern(LightCube winningBlock, Color[] winColors) {
      if (winColors[0] == Color.white) {
        _SpeedTapGame.StartCycleCubeSingleColor(winningBlock, winColors, _kWinCycleSpeed, Color.black);
      }
      else {
        _SpeedTapGame.StartCycleCubeSingleColor(winningBlock, winColors, _kWinCycleSpeed, Color.white);
      }
    }

    private void SetLosingLightPattern(LightCube losingBlock, bool madeMistake) {
      if (madeMistake) {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapRed);
        losingBlock.SetFlashingLEDs(Color.red, 100, 100, 0);
      }
      else {
        losingBlock.SetLEDs(0);
      }
    }

    private void ClearWinningLightPatterns() {
      _SpeedTapGame.StopCycleCube(_SpeedTapGame.PlayerBlock);
      _SpeedTapGame.StopCycleCube(_SpeedTapGame.CozmoBlock);
    }

    private void ReactToHand() {
      if (_CurrentWinner == PointWinner.Player) {
        // TODO add event listener
        AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapHandLose, HandleHandEndAnimDone);
        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapHandLose);
      }
      else {
        // TODO add event listener
        AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapHandWin, HandleHandEndAnimDone);
        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapHandWin);
      }
    }

    private void HandleRoundEnd() {
      GameAudioClient.SetMusicState(_SpeedTapGame.BetweenRoundsMusic);
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedRoundEnd);

      _SpeedTapGame.UpdateRoundScore();
      _SpeedTapGame.UpdateUI();

      if (_SpeedTapGame.IsSessionComplete()) {
        _SpeedTapGame.UpdateUIForSessionEnd();
        if (_CurrentWinner == PointWinner.Player) {
          // TODO add event listener
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapSessionLose, HandleSessionAnimDone);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapSessionLose);
        }
        else {
          // TODO add event listener
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapSessionWin, HandleSessionAnimDone);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapSessionWin);
        }
      }
      else {
        if (_CurrentWinner == PointWinner.Player) {
          // TODO add event listener
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapRoundLose, HandleRoundEndAnimDone);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapRoundLose);
        }
        else {
          // TODO add event listener
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapRoundWin, HandleRoundEndAnimDone);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapRoundWin);
        }
      }
    }

    private void HandleHandEndAnimDone(bool success) {
      _StateMachine.SetNextState(new SpeedTapHandCubesOff());
      ClearWinningLightPatterns();
    }

    private void HandleRoundEndAnimDone(bool success) {
      _SpeedTapGame.ResetScore();
      ClearWinningLightPatterns();
      _StateMachine.SetNextState(new SpeedTapCozmoDriveToCube(false));
    }

    private void HandleSessionAnimDone(bool success) {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedEnd);
      ClearWinningLightPatterns();
      _SpeedTapGame.HandleSessionEnd();
    }
  }
}
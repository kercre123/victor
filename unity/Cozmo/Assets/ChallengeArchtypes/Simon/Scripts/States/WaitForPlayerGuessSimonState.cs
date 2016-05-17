using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;

namespace Simon {
  public class WaitForPlayerGuessSimonState : State {

    private SimonGame _GameInstance;
    private IList<int> _SequenceList;
    private int _CurrentSequenceIndex = 0;
    private float _LastTappedTime;
    private int _TargetCube = -1;
    private uint _TargetCubeColor;
    private float _StartLightBlinkTime = -1;
    private const float _kTapBufferSeconds = 0f;

    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += OnBlockTapped;
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kSimonGameHeaderMakeYourGuess);
      _GameInstance.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kSimonGameLabelMakeYourGuess);
      _GameInstance.SharedMinigameView.CozmoScoreboard.Dim = true;
      _GameInstance.SharedMinigameView.PlayerScoreboard.Dim = false;
      _SequenceList = _GameInstance.GetCurrentSequence();
      _CurrentRobot.SetHeadAngle(1.0f);
      // add delay before allowing player taps because cozmo can accidentally tap when setting pattern.
      _LastTappedTime = Time.time;
    }

    public override void Update() {
      base.Update();
      if (_StartLightBlinkTime == -1) {
        if (_CurrentSequenceIndex == _SequenceList.Count) {
          PlayerWinGame();
        }
        if (_TargetCube != -1) {
          _CurrentRobot.DriveWheels(0f, 0f);
          if (_SequenceList[_CurrentSequenceIndex] == _TargetCube) {
            _CurrentSequenceIndex++;
          }
          else {
            PlayerLoseGame();
          }
          _TargetCube = -1;
        }
      }
      else if (Time.time - _StartLightBlinkTime > SimonGame.kLightBlinkLengthSeconds) {
        _StartLightBlinkTime = -1;
      }
    }

    public override void Exit() {
      base.Exit();
      LightCube.TappedAction -= OnBlockTapped;
      _CurrentRobot.DriveWheels(0f, 0f);
    }

    private void HandleOnPlayerWinAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Human));
    }

    private void HandleOnPlayerLoseAnimationDone(bool success) {
      _GameInstance.RaiseMiniGameLose(Localization.GetWithArgs(
        LocalizationKeys.kSimonGameTextPatternLength, _SequenceList.Count));
    }

    private void PlayerLoseGame() {
      _GameInstance.SetCubeLightsGuessWrong();

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kWin, HandleOnPlayerLoseAnimationDone));
    }

    private void PlayerWinGame() {
      _GameInstance.SetCubeLightsGuessRight();

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);

      // TODO: Need to find a better animation than shocked; Cozmo should be determined to win 
      // and feel a bit thwarted 
      _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, HandleOnPlayerWinAnimationDone));
    }

    private void OnBlockTapped(int id, int times) {
      _CurrentRobot.SetHeadAngle(Random.Range(CozmoUtil.kIdealBlockViewHeadValue, 0f));
      if (Time.time - _LastTappedTime < _kTapBufferSeconds || _StartLightBlinkTime != -1) {
        return;
      }

      _LastTappedTime = Time.time;
      _StartLightBlinkTime = Time.time;

      SimonGame game = _StateMachine.GetGame() as SimonGame;
      GameAudioClient.PostAudioEvent(game.GetAudioForBlock(id));
      game.BlinkLight(id, SimonGame.kLightBlinkLengthSeconds, Color.black, game.GetColorForBlock(id));

      _TargetCube = id;
      LightCube cube = _CurrentRobot.LightCubes[_TargetCube];
      _CurrentRobot.TurnTowardsObject(cube, false, SimonGame.kTurnSpeed_rps, SimonGame.kTurnAccel_rps2);
    }
  }

}

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
    private const float _kTapBufferSeconds = 0.1f;
    private bool _IsAnimating = false;

    List<CubeTapTime> _BadTapLists = new List<CubeTapTime>();
    public class CubeTapTime {
      public CubeTapTime(int setID, float setTimeStamp) {
        id = setID;
        timeStamp = setTimeStamp;
      }
      public float timeStamp;
      public int id;
    }

    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += OnBlockTapped;
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _SequenceList = _GameInstance.GetCurrentSequence();
      _CurrentRobot.SetHeadAngle(1.0f);
      // add delay before allowing player taps because cozmo can accidentally tap when setting pattern.
      _LastTappedTime = Time.time;

      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonCozmoWin, HandleOnPlayerLoseAnimationDone);
      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonPlayerHandComplete, HandleOnPlayerWinAnimationDone);

      _GameInstance.OnTurnStage(PlayerType.Human, false);
    }

    public override void Exit() {
      base.Exit();
      LightCube.TappedAction -= OnBlockTapped;
      _CurrentRobot.DriveWheels(0f, 0f);

      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonCozmoWin, HandleOnPlayerLoseAnimationDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonPlayerHandComplete, HandleOnPlayerWinAnimationDone);
    }

    public override void Update() {
      base.Update();
      if (_StartLightBlinkTime == -1 && !_IsAnimating) {
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
        for (int i = 0; i < _BadTapLists.Count; ++i) {
          if (_BadTapLists[i].timeStamp < Time.time) {
            // This will clear the list and ready us for next time
            DoBlockTap(_BadTapLists[i].id);
            break;
          }
        }
      }
      else if (Time.time - _StartLightBlinkTime > SimonGame.kLightBlinkLengthSeconds) {
        _StartLightBlinkTime = -1;
      }
    }

    private void HandleOnPlayerWinAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Cozmo));
    }

    private void HandleOnPlayerLoseAnimationDone(bool success) {
      _GameInstance.RaiseMiniGameLose(Localization.GetWithArgs(
        LocalizationKeys.kSimonGameTextPatternLength, (_SequenceList.Count - _GameInstance.MinSequenceLength + 1)));
    }

    private void PlayerLoseGame() {
      _GameInstance.SetCubeLightsGuessWrong(_SequenceList[_CurrentSequenceIndex], _TargetCube);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSimonCozmoWin);
      _IsAnimating = true;

      _GameInstance.ShowBanner(LocalizationKeys.kSimonGameLabelCozmoWin);
    }

    private void PlayerWinGame() {
      _GameInstance.SetCubeLightsGuessRight();

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);

      // AnimationManager will send callback to HandleOnPlayerWinAnimationDone
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSimonPlayerHandComplete);
      _IsAnimating = true;

      _GameInstance.ShowBanner(LocalizationKeys.kSimonGameLabelCorrect);
    }

    private void OnBlockTapped(int id, int times, float timeStamp) {
      if (Time.time - _LastTappedTime < _kTapBufferSeconds || _StartLightBlinkTime != -1) {
        return;
      }

      // Just playing the ending animation
      if (_IsAnimating) {
        return;
      }

      // Only ignore incorrect taps from punching table, give the benifit of the doubt
      // correct ID, process immediately
      if (id == _SequenceList[_CurrentSequenceIndex]) {
        DoBlockTap(id);
      }
      else {
        // Add to a list and set some timers
        _BadTapLists.Add(new CubeTapTime(id, Time.time + _kTapBufferSeconds));
      }

    }

    private void DoBlockTap(int id) {
      _BadTapLists.Clear();
      _CurrentRobot.SetHeadAngle(Random.Range(CozmoUtil.kIdealBlockViewHeadValue, 0f));
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

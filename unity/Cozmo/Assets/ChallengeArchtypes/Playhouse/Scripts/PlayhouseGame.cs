using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Playhouse {
  public class PlayhouseGame : GameBase {

    private PlayhouseGameConfig _Config;
    private List<string> _AnimationSequence = new List<string>();
    private int _SequenceIndex = -1;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = (PlayhouseGameConfig)minigameConfig;

      State nextState = new RequestPlay();
      InitialCubesState initCubeState = new InitialCubesState(new HowToPlayState(nextState), _Config.NumCubesRequired());
      _StateMachine.SetNextState(initCubeState);

    }

    protected override void CleanUpOnDestroy() {

    }

    public void RequestAnimationDone() {
      // pop up UI for creating animation sequence.
    }


    private void HandleRunSequence() {
      CurrentRobot.SendAnimation(AnimationName.kCodeBreakerNewIdea, OnSequenceAnimationDone);
      _SequenceIndex = -1;
    }

    private void OnSequenceAnimationDone(bool success) {
      _SequenceIndex++;
      if (_SequenceIndex < _AnimationSequence.Count) {
        CurrentRobot.SendAnimation(_AnimationSequence[_SequenceIndex], OnSequenceAnimationDone);
      }
      else {
        ResetGame();
      }
    }

    private void ResetGame() {
      _StateMachine.SetNextState(new RequestPlay());
      _AnimationSequence.Clear();
    }
  }
}

using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Playhouse {
  public class PlayhouseGame : GameBase {

    [SerializeField]
    private PlayhousePanel _PlayhousePanelPrefab;
    private PlayhousePanel _PlayhousePanel = null;

    private List<string> _AnimationSequence;
    private int _SequenceIndex = -1;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      State nextState = new RequestPlay();
      HowToPlayState howToPlayState = new HowToPlayState(nextState);
      _StateMachine.SetNextState(howToPlayState);

    }

    protected override void CleanUpOnDestroy() {
    }

    public void RequestAnimationDone() {
      // pop up UI for creating animation sequence.
      _PlayhousePanel = SharedMinigameView.ShowWideGameStateSlide(_PlayhousePanelPrefab.gameObject, "playhouse_panel").GetComponent<PlayhousePanel>();
      _PlayhousePanel.Initialize(HandleRunSequence);
    }

    private void HandleRunSequence() {
      _AnimationSequence = _PlayhousePanel.GetAnimationList();
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
      SharedMinigameView.HideGameStateSlide();
    }
  }
}

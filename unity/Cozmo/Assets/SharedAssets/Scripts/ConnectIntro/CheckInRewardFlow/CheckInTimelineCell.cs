using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using Cozmo.UI;
using DG.Tweening;

public class CheckInTimelineCell : MonoBehaviour {

  [SerializeField]
  private float _TextFadeAlpha = 0.5f;

  public Action<int> OnAnimationComplete;

  [SerializeField]
  private Image _Envelope;

  [SerializeField]
  private Text _Text;

  [SerializeField]
  private Animator _LeftProgressBarAnimator;

  [SerializeField]
  private Animator _RightProgressBarAnimator;

  [SerializeField]
  private Animator _CheckmarkAnimator;

  [SerializeField]
  private Anki.Cozmo.Audio.AudioEventParameter _CheckSound = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;

  private int _Index = 0;
  private bool _IsLongerThanMaxStreak;
  private bool _IsFirstCell = false;
  private bool _IsPenultimateCell = false;
  private bool _IsFinalCell = false;
  /// <summary>
  /// Initializes the cell.
  /// </summary>
  /// <returns>The cell.</returns>
  /// <param name="currDay">Curr day for the Streak.</param>
  /// <param name="streakMet">Streak met, if false then envelope doesn't show.</param>
  public void InitializeCell(int currDay, bool streakMet, bool penultimateInList, bool lastInList, int index) {
    _Index = index;
    _IsFirstCell = (index == 0);
    _IsPenultimateCell = penultimateInList;
    _IsFinalCell = lastInList;

    if (streakMet) {
      // If the first in the list and then 7 streak, make first bar full
      _IsLongerThanMaxStreak = currDay > 1;
    }
    else {
      // If completely untouched, hide the envelope
      _Envelope.gameObject.SetActive(false);
      _Text.color = new Color(_Text.color.r, _Text.color.g, _Text.color.b, _TextFadeAlpha);
    }

    _Text.text = Localization.GetWithArgs(LocalizationKeys.kLabelDayCount, currDay);
  }

  private void OnDestroy() {
    _RightProgressBarAnimator.StopListenForAnimationEnd(HandleRightProgressBarAnimationFinished);
    _LeftProgressBarAnimator.StopListenForAnimationEnd(HandleLeftProgressBarAnimationFinished);
  }

  public void PlayAnimation() {
    if (_IsFirstCell && _IsLongerThanMaxStreak) {
      PlayLeftBarAnimation();
    }
    else {
      PlayCheckmarkAndRightProgressBarAnimation();
    }
  }

  private void PlayLeftBarAnimation() {
    if (_IsFirstCell && _IsFinalCell) {
      _LeftProgressBarAnimator.SetBool("PlayOutQuadAnimation", true);
    }
    else {
      _LeftProgressBarAnimator.SetBool("PlayLinearAnimation", true);
    }
    // Listen for finish
    _LeftProgressBarAnimator.ListenForAnimationEnd(HandleLeftProgressBarAnimationFinished);
  }

  private void HandleLeftProgressBarAnimationFinished(AnimatorStateInfo animatorState) {
    // If name matches
    if (animatorState.IsName("CheckInLeftProgressBarLinearAnimation")
        || animatorState.IsName("CheckInLeftProgressBarQuadAnimation")) {
      _LeftProgressBarAnimator.StopListenForAnimationEnd(HandleLeftProgressBarAnimationFinished);
      PlayCheckmarkAndRightProgressBarAnimation();
    }
  }

  // Play checkmark animation and right bar if necessary
  private void PlayCheckmarkAndRightProgressBarAnimation() {
    PlayCheckmarkAnimation();

    if (!_IsFinalCell) {
      PlayRightBarAnimation();
    }
    else {
      ListenForCheckmarkAnimationEnd();
    }
  }

  private void PlayCheckmarkAnimation() {
    _CheckmarkAnimator.SetBool("PlayGrowAnimation", true);
    Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_CheckSound);
  }

  private void ListenForCheckmarkAnimationEnd() {
    _CheckmarkAnimator.ListenForAnimationEnd(HandleLastCheckmarkAnimationFinished);
  }

  private void HandleLastCheckmarkAnimationFinished(AnimatorStateInfo animatorState) {
    if (animatorState.IsName("CheckInCheckmarkGrowAnimation")) {
      _CheckmarkAnimator.StopListenForAnimationEnd(HandleLastCheckmarkAnimationFinished);
      RaiseAnimationComplete();
    }
  }

  private void PlayRightBarAnimation() {
    _RightProgressBarAnimator.ListenForAnimationEnd(HandleRightProgressBarAnimationFinished);
    if (_IsPenultimateCell) {
      _RightProgressBarAnimator.SetBool("PlayOutQuadAnimation", true);
    }
    else {
      _RightProgressBarAnimator.SetBool("PlayLinearAnimation", true);
    }
  }

  private void HandleRightProgressBarAnimationFinished(AnimatorStateInfo animatorState) {
    if (animatorState.IsName("CheckInProgressBarLinearAnimation")
        || animatorState.IsName("CheckInProgressBarOutQuadAnimation")) {
      _RightProgressBarAnimator.StopListenForAnimationEnd(HandleRightProgressBarAnimationFinished);
      RaiseAnimationComplete();
    }
  }

  private void RaiseAnimationComplete() {
    if (OnAnimationComplete != null) {
      OnAnimationComplete.Invoke(_Index);
    }
  }
}

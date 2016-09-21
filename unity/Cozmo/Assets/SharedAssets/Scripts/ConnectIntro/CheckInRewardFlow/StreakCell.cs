using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using Cozmo.UI;
using DG.Tweening;

public class StreakCell : MonoBehaviour {

  [SerializeField]
  private float _CheckMarkMaxScale = 1.25f;

  [SerializeField]
  private float _CheckMarkPopDuration = 0.25f;

  [SerializeField]
  private float _CheckMarkSettleDuration = 0.5f;

  [SerializeField]
  private float _TextFadeAlpha = 0.5f;

  public Action<int> OnAnimationComplete;

  [SerializeField]
  private Image _Envelope;
  [SerializeField]
  private Image _CheckMark;
  [SerializeField]
  private Text _Text;
  public ProgressBar ProgBar;

  [SerializeField]
  private Anki.Cozmo.Audio.AudioEventParameter _CheckSound = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;

  private int _Index = 0;
  public bool IsFinalCell = false;
  /// <summary>
  /// Initializes the cell.
  /// </summary>
  /// <returns>The cell.</returns>
  /// <param name="currDay">Curr day for the Streak.</param>
  /// <param name="streakMet">Streak met, if false then envelope doesn't show.</param>
  public void InitializeCell(int currDay, bool streakMet, bool lastInList, int index) {
    _Index = index;
    _CheckMark.transform.localScale = Vector3.zero;
    _CheckMark.gameObject.SetActive(false);
    // If the first in the list and than 7 streak, make first bar full
    if ((index == 0) && (currDay > 1)) {
      ProgBar.SetProgress(1.0f, true);
    }
    else {
      ProgBar.SetProgress(0.0f, true);
      ProgBar.ProgressUpdateCompleted += BeginCheckSequence;
    }
    // If completely untouched, hide the envelope. Tint text and recolor shiz if 
    if ((streakMet == false) && (lastInList == false)) {
      _Envelope.gameObject.SetActive(false);
      _Text.color = new Color(_Text.color.r, _Text.color.g, _Text.color.b, _TextFadeAlpha);
    }

    _Text.text = Localization.GetWithArgs(LocalizationKeys.kLabelDayCount, currDay);
  }

  public void BeginProgFill() {
    ProgBar.SetProgress(1.0f);
  }

  public void BeginCheckSequence() {
    _CheckMark.gameObject.SetActive(true);
    Sequence checkSequence = DOTween.Sequence();
    checkSequence.Join(_CheckMark.transform.DOScale(_CheckMarkMaxScale, _CheckMarkPopDuration).OnStart(() => {
      Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_CheckSound);
    }));
    checkSequence.Append(_CheckMark.transform.DOScale(1.0f, _CheckMarkSettleDuration));
    checkSequence.AppendCallback(HandleCheckMarkComplete);
  }

  private void HandleCheckMarkComplete() {
    if (OnAnimationComplete != null) {
      OnAnimationComplete.Invoke(_Index);
    }
  }

  void OnDestroy() {
    if (ProgBar.ProgressUpdateCompleted != null) {
      ProgBar.ProgressUpdateCompleted -= BeginCheckSequence;
    }
  }

}

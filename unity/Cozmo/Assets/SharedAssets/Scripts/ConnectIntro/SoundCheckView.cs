using UnityEngine;
using System.Collections;
using DG.Tweening;

public class SoundCheckView : Cozmo.UI.BaseView {

  public System.Action OnSoundCheckComplete;

  [SerializeField]
  private Cozmo.UI.CozmoButton _PlayButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _PlayAgainButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SoundsGoodButton;

  [SerializeField]
  private UnityEngine.UI.Image[] _SoundFillImages;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _SoundTextConfirm;

  [SerializeField]
  private float _SoundCheckAudioDuration = 4.216f;

  private bool _PlayedOnce = false;

  private Sequence _SoundTween = null;

  private void Awake() {
    _PlayButton.Initialize(HandlePlayButton, "play_sound_button", this.DASEventViewName);
    _SoundsGoodButton.Initialize(HandleSoundsGoodButton, "sounds_good_button", this.DASEventViewName);
    _PlayAgainButton.Initialize(HandlePlayButton, "play_again_button", this.DASEventViewName);
    _SoundTextConfirm.gameObject.SetActive(false);
  }

  private void Start() {
    _PlayButton.Text = Localization.Get(LocalizationKeys.kButtonPlay);
    _PlayAgainButton.gameObject.SetActive(false);
    _SoundsGoodButton.gameObject.SetActive(false);
  }

  private void OnDestroy() {
    if (_SoundTween != null) {
      _SoundTween.Kill();
    }
  }

  private void HandlePlayButton() {
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Onboarding_Test_Tone);

    // TODO: use actual audio finish trigger?
    Invoke("HandlePlaySoundComplete", _SoundCheckAudioDuration);

    _SoundTween = DOTween.Sequence();
    for (int i = 0; i < _SoundFillImages.Length; i++) {
      // Apparently dotween needs this step to not barf
      int index = i;
      _SoundFillImages[index].fillAmount = 0.0f;
      _SoundTween.Insert(0f, DOTween.To(() => _SoundFillImages[index].fillAmount,
                                        x => _SoundFillImages[index].fillAmount = x,
                                        1.0f,
                                        _SoundCheckAudioDuration));
    }

    _PlayButton.Interactable = false;
    _PlayAgainButton.Interactable = false;
    _SoundsGoodButton.Interactable = false;
  }

  private void HandlePlaySoundComplete() {
    _SoundTextConfirm.gameObject.SetActive(true);

    _PlayAgainButton.Interactable = true;
    _SoundsGoodButton.Interactable = true;

    if (_PlayedOnce == false) {
      _SoundsGoodButton.Text = Localization.Get(LocalizationKeys.kButtonSoundsGood);
      _PlayAgainButton.Text = Localization.Get(LocalizationKeys.kButtonAgain);

      _PlayButton.gameObject.SetActive(false);
      _PlayAgainButton.gameObject.SetActive(true);
      _SoundsGoodButton.gameObject.SetActive(true);

      _PlayedOnce = true;
    }
  }

  private void HandleSoundsGoodButton() {
    StopAllCoroutines();
    if (OnSoundCheckComplete != null) {
      OnSoundCheckComplete();
    }
  }

  protected override void CleanUp() {

  }
}

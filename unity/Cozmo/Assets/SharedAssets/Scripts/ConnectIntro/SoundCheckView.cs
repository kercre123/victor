using UnityEngine;
using System.Collections;

public class SoundCheckView : Cozmo.UI.BaseView {

  public System.Action OnSoundCheckComplete;

  [SerializeField]
  private Cozmo.UI.CozmoButton _PlayButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _PlayAgainButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SoundsGoodButton;

  private bool _PlayedOnce = false;

  private void Awake() {
    _PlayButton.Initialize(HandlePlayButton, "play_sound_button", this.DASEventViewName);
    _SoundsGoodButton.Initialize(HandleSoundsGoodButton, "sounds_good_button", this.DASEventViewName);
    _PlayAgainButton.Initialize(HandlePlayButton, "play_again_button", this.DASEventViewName);
  }

  private void Start() {
    _PlayButton.Text = Localization.Get(LocalizationKeys.kButtonPlay);
    _PlayAgainButton.gameObject.SetActive(false);
    _SoundsGoodButton.gameObject.SetActive(false);
  }

  private void HandlePlayButton() {
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Win);
    // TODO: use actual audio finish trigger?
    Invoke("HandlePlaySoundComplete", 1.5f);
    _PlayButton.Interactable = false;
    _PlayAgainButton.Interactable = false;
  }

  private void HandlePlaySoundComplete() {
    _PlayAgainButton.Interactable = true;
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

using UnityEngine;
using System.Collections;
using Anki.UI;

namespace Cozmo.UI {
  public class CozmoButton : AnkiButton {

    [SerializeField]
    private AnkiAnimateGlint _GlintAnimator;

    [SerializeField]
    private Anki.Cozmo.Audio.AudioEventParameter _UISoundEvent = Anki.Cozmo.Audio.AudioEventParameter.DefaultClick;

    public Anki.Cozmo.Audio.AudioEventParameter SoundEvent {
      get { return _UISoundEvent; }
      set { _UISoundEvent = value; }
    }

    protected override void Start() {
      base.Start();
      base.onClick.AddListener(HandleOnPress);
    }

    protected override void OnDestroy() {
      base.OnDestroy();
      base.onClick.RemoveListener(HandleOnPress);
    }

    protected override void ShowEnabledState() {
      base.ShowEnabledState();
      ShowGlint(true);
    }

    protected override void ShowPressedState() {
      base.ShowPressedState();
      ShowGlint(true);
    }

    protected override void ShowDisabledState() {
      base.ShowDisabledState();
      ShowGlint(false);
    }

    private void HandleOnPress() {
      Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_UISoundEvent);
    }

    private void ShowGlint(bool show) {
      if (_GlintAnimator != null) {
        _GlintAnimator.EnableGlint(show);
      }
    }
  }

}

using UnityEngine;
using System.Collections;
using Anki.UI;

namespace Cozmo.UI {
  public class CozmoButton : AnkiButton {

    [SerializeField]
    private AnkiAnimateGlint _GlintAnimator;

    [SerializeField]
    private Anki.Cozmo.Audio.AudioEventParameter _UISoundEvent = Anki.Cozmo.Audio.AudioEventParameter.DefaultClick;

    [SerializeField]
    protected bool _ShowDisabledStateWhenInteractable = false;

    public bool ShowDisabledStateWhenInteractable {
      get { return _ShowDisabledStateWhenInteractable; }
      set {
        _ShowDisabledStateWhenInteractable = value;
        UpdateVisuals();
      }
    }

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

    protected override void UpdateVisuals() {
      if (_ShowDisabledStateWhenInteractable) {
        InitializeDefaultGraphics();
        ShowDisabledState();
      }
      else {
        base.UpdateVisuals();
      }
    }

    protected override void ShowEnabledState() {
      if (_ShowDisabledStateWhenInteractable) {
        ShowDisabledState();
      }
      else {
        base.ShowEnabledState();
        ShowGlint(true);
      }
    }

    protected override void ShowPressedState() {
      if (_ShowDisabledStateWhenInteractable) {
        ShowDisabledState();
      }
      else {
        base.ShowPressedState();
        ShowGlint(true);
      }
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

    public void SetButtonGraphic(Sprite sprite) {
      foreach (var graphic in ButtonGraphics) {
        graphic.enabledSprite = sprite;
        graphic.disabledSprite = sprite;
        graphic.pressedSprite = sprite;
      }

      UpdateVisuals();
    }

    public void SetButtonTint(Color tintColor) {
      foreach (var graphic in ButtonGraphics) {
        float oldAlpha = graphic.enabledColor.a;
        graphic.enabledColor = tintColor;
        graphic.enabledColor.a = oldAlpha;

        oldAlpha = graphic.pressedColor.a;
        graphic.pressedColor = tintColor;
        graphic.pressedColor.a = oldAlpha;

        graphic.disabledColor = tintColor * graphic.disabledColor;
      }

      UpdateVisuals();
    }
  }
}

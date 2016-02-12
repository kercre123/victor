using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;
using UnityEngine.Serialization;
using UnityEngine.UI;
using Anki.UI;
using Cozmo.UI;

namespace Anki {
  namespace UI {
    [AddComponentMenu("Anki/Button", 02)]
    [ExecuteInEditMode]
    public class AnkiButton : UIBehaviour, IPointerClickHandler, IPointerDownHandler, IPointerUpHandler {

      [SerializeField]
      private bool _Interactable = true;

      public bool Interactable {
        get { return _Interactable; }
        set { 
          if (value != _Interactable) {
            _Interactable = value; 

            UpdateVisuals();
          }
        }
      }

      [Serializable]
      public class ButtonClickedEvent : UnityEvent {
      }

      [Serializable]
      public class ButtonDownEvent : UnityEvent {
      }

      [Serializable]
      public class ButtonUpEvent : UnityEvent {
      }

      private ButtonClickedEvent _OnClick = new ButtonClickedEvent();

      private ButtonDownEvent _OnPress = new ButtonDownEvent();

      private ButtonUpEvent _OnRelease = new ButtonUpEvent();

      [SerializeField]
      private string _DASEventName;

      [SerializeField]
      private AnkiTextLabel _TextLabel;

      [SerializeField]
      private float _TextLabelOffset = -5f;
      private Vector3? _TextDefaultPosition = null;
      private Vector3 _TextPressedPosition;

      public Color TextEnabledColor = Color.white;

      public Color TextPressedColor = Color.gray;

      public Color TextDisabledColor = Color.gray;

      [SerializeField]
      private CanvasGroup _AlphaController;

      [SerializeField]
      private AnkiAnimateGlint _GlintAnimator;

      public AnkiButtonImage[] ButtonGraphics;

      public string Text {
        get {
          if (_TextLabel == null) {
            return null;
          }
          return _TextLabel.text;
        }
        set { 
          if (_TextLabel != null) {
            _TextLabel.text = value;
          }
        }
      }

      public ButtonClickedEvent onClick {
        get { return _OnClick; }
        set { _OnClick = value; }
      }

      public ButtonDownEvent onPress {
        get { return _OnPress; }
        set { _OnPress = value; }
      }

      public ButtonUpEvent onRelease {
        get { return _OnRelease; }
        set { _OnRelease = value; }
      }

      public string DASEventName {
        get { return _DASEventName; }
        set { _DASEventName = value; }
      }

      public float Alpha {
        get { return _AlphaController != null ? _AlphaController.alpha : -1; }
        set {
          if (_AlphaController != null) {
            _AlphaController.alpha = value;
          }
        }
      }


      [SerializeField]
      private Anki.Cozmo.Audio.AudioEventParameter 
      _UISoundEvent = Anki.Cozmo.Audio.AudioEventParameter.DefaultClick;

      public Anki.Cozmo.Audio.AudioEventParameter SoundEvent {
        get { return _UISoundEvent; }
        set { _UISoundEvent = value; }
      }


      public bool IsInteractable() {
        return _Interactable;
      }

      private bool _IsInitialized = false;

      protected override void Start() {
        base.Start();
        UpdateVisuals();
      }

      private void InitializeDefaultGraphics() {
        if (!_IsInitialized) {
          foreach (AnkiButtonImage graphic in ButtonGraphics) {
            if (graphic.targetImage != null) {
              graphic.enabledSprite = graphic.targetImage.sprite;
              graphic.enabledColor = graphic.targetImage.color;
            }
            else {
              DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name);
            }
          }
          _IsInitialized = true;
        }
      }

      protected override void OnEnable() {
        base.OnEnable();

        // Grab the text label while in editor
        if (_TextLabel == null) {
          _TextLabel = GetComponentInChildren<AnkiTextLabel>();
        }
      }

      private bool DisableInteraction() {
        return !IsActive() || !IsInteractable();
      }

      private void Tap() {
        if (DisableInteraction()) {
          return;
        }

        // Set to pressed visual state
        ShowPressedState();
        StartCoroutine(DelayedResetButton());
    
        PlayAudioEvent();
        _OnClick.Invoke();
      }

      private System.Collections.IEnumerator DelayedResetButton() {
        yield return new WaitForSeconds(0.1f);
        UpdateVisuals();
      }

      private void Press() {
        if (DisableInteraction()) {
          return;
        }

        SetUpTextOffset();

        // Set to pressed visual state
        ShowPressedState();

        _OnPress.Invoke();
      }

      private void SetUpTextOffset() {
        if (_TextLabel != null && !_TextDefaultPosition.HasValue) {
          _TextDefaultPosition = _TextLabel.gameObject.transform.localPosition;
          _TextPressedPosition = _TextLabel.gameObject.transform.localPosition;
          _TextPressedPosition.y += _TextLabelOffset;
        }
      }

      private void Release() {
        if (DisableInteraction()) {
          return;
        }

        // Reset to normal visual state
        ShowEnabledState();
    
        _OnRelease.Invoke();
      }
  
      // Trigger all registered callbacks.
      public virtual void OnPointerClick(PointerEventData eventData) {
        if (eventData.button != PointerEventData.InputButton.Left) {
          return;
        }

        if (_TextLabel != null && _TextLabel.text.StartsWith("#")) {
          DAS.Event("ui.button", _TextLabel.text, new Dictionary<string,string>{ { "$data", _DASEventName } });
        }
        else {
          DAS.Event("ui.button", this.name, new Dictionary<string,string>{ { "$data", _DASEventName } });
        }

        Tap();
      }

      public void OnPointerDown(PointerEventData eventData) {
        DAS.Debug("AnkiButton.OnPointerDown", string.Format("{0} Pressed", _DASEventName));

        Press();
      }

      public void OnPointerUp(PointerEventData eventData) {
        DAS.Debug("AnkiButton.OnPointerUp", string.Format("{0} Released", _DASEventName));

        Release();
      }

      public void PlayAudioEvent() {
        if (DisableInteraction()) {
          return;
        }

        Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_UISoundEvent);
      }

      private void UpdateVisuals() {
        InitializeDefaultGraphics();
        if (IsInteractable()) {
          ShowEnabledState();
        }
        else {
          ShowDisabledState();
        }
      }

      private void ShowEnabledState() {
        if (ButtonGraphics != null) {
          foreach (AnkiButtonImage graphic in ButtonGraphics) {
            if (graphic.targetImage != null && graphic.enabledSprite != null) {
              SetGraphic(graphic, graphic.enabledSprite, graphic.enabledColor, graphic.ignoreSprite);
            }
            else {
              DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name);
            }
          }
        }

        ResetTextPosition(TextEnabledColor);
        ShowGlint(true);
      }

      private void ShowPressedState() {
        foreach (AnkiButtonImage graphic in ButtonGraphics) {
          if (graphic.targetImage != null && graphic.enabledSprite != null) {
            SetGraphic(graphic, graphic.pressedSprite, graphic.pressedColor, graphic.ignoreSprite);
          }
          else {
            DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name);
          }
        }

        PressTextPosition();
        ShowGlint(true);
      }

      private void ShowDisabledState() {
        foreach (AnkiButtonImage graphic in ButtonGraphics) {
          if (graphic.targetImage != null && graphic.enabledSprite != null) {
            SetGraphic(graphic, graphic.disabledSprite, graphic.disabledColor, graphic.ignoreSprite);
          }
          else {
            DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name);
          }
        }

        ResetTextPosition(TextDisabledColor);
        ShowGlint(false);
      }

      private void SetGraphic(AnkiButtonImage graphic, Sprite desiredSprite, Color desiredColor, bool ignoreSprite) {
        if (!ignoreSprite) {
          graphic.targetImage.overrideSprite = desiredSprite ?? graphic.enabledSprite;
        }
        graphic.targetImage.color = desiredColor;
      }

      private void ResetTextPosition(Color color) {
        if (_TextLabel != null) {
          _TextLabel.color = color;
          if (_TextDefaultPosition.HasValue) {
            _TextLabel.transform.localPosition = _TextDefaultPosition.Value;
          }
        }
      }

      private void PressTextPosition() {
        if (_TextLabel != null) {
          _TextLabel.color = TextPressedColor;
          _TextLabel.transform.localPosition = _TextPressedPosition;
        }
      }

      private void ShowGlint(bool show) {
        if (_GlintAnimator != null) {
          _GlintAnimator.EnableGlint(show);
        }
      }

      [Serializable]
      public class AnkiButtonImage {
        public Image targetImage;
        public bool ignoreSprite = false;

        [NonSerialized]
        public Sprite enabledSprite;

        [NonSerialized]
        public Color enabledColor = Color.white;

        public Sprite pressedSprite;
        public Color pressedColor = Color.gray;
        public Sprite disabledSprite;
        public Color disabledColor = Color.gray;
      }
    }
  }
}

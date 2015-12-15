using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;
using UnityEngine.Serialization;
using UnityEngine.UI;
using Anki.UI;

namespace Anki {
  namespace UI {
    [AddComponentMenu("Anki/Button", 02)]
    [ExecuteInEditMode]
    public class AnkiButton : UIBehaviour, IPointerClickHandler, IPointerDownHandler, IPointerUpHandler {

      [SerializeField]
      private bool _Interactable = true;

      public bool interactable {
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
      private Vector3 _TextDefaultPosition;
      private Vector3 _TextPressedPosition;

      public Color TextEnabledColor = Color.white;

      public Color TextPressedColor = Color.gray;

      public Color TextDisabledColor = Color.gray;

      [SerializeField]
      private CanvasGroup _AlphaController;

      public AnkiButtonImage[] ButtonGraphics;

      public string Text {
        get{ return _TextLabel.text; }
        set { 
          _TextLabel.text = value;
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

      /*
      [SerializeField]
      private AudioController.UIButtonAudioEvent
      _UISoundEvent = AudioController.UIButtonAudioEvent.SelectSmall;

      public AudioController.UIButtonAudioEvent SoundEvent {
        get { return _UISoundEvent; }
        set { _UISoundEvent = value; }
      }
      */

      public bool IsInteractable() {
        return _Interactable;
      }

      protected override void Start() {
        base.Start();
        _TextDefaultPosition = _TextLabel.gameObject.transform.localPosition;
        _TextPressedPosition = _TextDefaultPosition;
        _TextPressedPosition.y += _TextLabelOffset;
        UpdateVisuals();
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

        // Set to pressed visual state
        ShowPressedState();

        _OnPress.Invoke();
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

        // TODO: Audio
        //DAS.Debug("AnkiButton", string.Format("Playing {0} sound", _UISoundEvent));
        //AudioController.PostUIButtonAudioEvent(_UISoundEvent);
      }

      private void UpdateVisuals() {
        if (IsInteractable()) {
          ShowEnabledState();
        }
        else {
          ShowDisabledState();
        }
      }

      private void ShowEnabledState() {
        foreach (AnkiButtonImage graphic in ButtonGraphics) {
          if (graphic.targetImage != null && graphic.enabledSprite != null) {
            SetGraphic(graphic, graphic.enabledSprite, graphic.enabledColor);
          }
          else {
            DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name);
          }
        }

        _TextLabel.color = TextEnabledColor;
        _TextLabel.transform.localPosition = _TextDefaultPosition;
      }

      private void ShowPressedState() {
        foreach (AnkiButtonImage graphic in ButtonGraphics) {
          if (graphic.targetImage != null && graphic.enabledSprite != null) {
            SetGraphic(graphic, graphic.pressedSprite, graphic.pressedColor);
          }
          else {
            DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name);
          }
        }
        _TextLabel.color = TextPressedColor;
        _TextLabel.transform.localPosition = _TextPressedPosition;
      }

      private void ShowDisabledState() {
        foreach (AnkiButtonImage graphic in ButtonGraphics) {
          if (graphic.targetImage != null && graphic.enabledSprite != null) {
            SetGraphic(graphic, graphic.disabledSprite, graphic.disabledColor);
          }
          else {
            DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name);
          }
        }
        _TextLabel.color = TextDisabledColor;
        _TextLabel.transform.localPosition = _TextDefaultPosition;
      }

      private void SetGraphic(AnkiButtonImage graphic, Sprite desiredSprite, Color desiredColor) {
        graphic.targetImage.overrideSprite = desiredSprite ?? graphic.enabledSprite;
        graphic.targetImage.color = desiredColor;
      }

      [Serializable]
      public class AnkiButtonImage {
        public Image targetImage;
        public Sprite enabledSprite;
        public Color enabledColor = Color.white;
        public Sprite pressedSprite;
        public Color pressedColor = Color.gray;
        public Sprite disabledSprite;
        public Color disabledColor = Color.gray;
      }
    }
  }
}

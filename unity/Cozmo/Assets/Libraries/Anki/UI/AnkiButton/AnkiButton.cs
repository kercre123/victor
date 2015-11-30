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
    [RequireComponent(typeof(AnkiWindow))]
    public class AnkiButton : Selectable, IPointerClickHandler {

      [Serializable]
      public class ButtonClickedEvent : UnityEvent {
      }

      [Serializable]
      public class ButtonDownEvent : UnityEvent {
      }

      [Serializable]
      public class ButtonUpEvent : UnityEvent {
      }

      [FormerlySerializedAs("onClick")]
      [SerializeField]
      private ButtonClickedEvent
        _OnClick = new ButtonClickedEvent();

      [FormerlySerializedAs("onPress")]
      [SerializeField]
      private ButtonDownEvent
        _OnPress = new ButtonDownEvent();

      [FormerlySerializedAs("onRelease")]
      [SerializeField]
      private ButtonUpEvent
        _OnRelease = new ButtonUpEvent();

      [SerializeField]
      private string
        _DASEventName;

      [SerializeField]
      private Text
        _TextLabel;

      [SerializeField]
      private Sprite
        _HighlightedSprite;

      [SerializeField]
      private Sprite
        _PressedSprite;

      [SerializeField]
      private Sprite
        _DisabledSprite;

      public Text SetTextRef {
        set{ _TextLabel = value; }
      }

      public string TextLabel {
        get{ return _TextLabel.text; }
        set { 
          _TextLabel.text = value;
        }
      }

      public Sprite HighlightedSprite {
        get { return _HighlightedSprite; }
        set { _HighlightedSprite = value; }
      }

      public Sprite PressedSprite {
        get { return _PressedSprite; }
        set { _PressedSprite = value; }
      }

      public Sprite DisabledSprite {
        get { return _DisabledSprite; }
        set { _DisabledSprite = value; }
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

      private AnkiWindow _ButtonView;

      /*
      [SerializeField]
      private AudioController.UIButtonAudioEvent
      _UISoundEvent = AudioController.UIButtonAudioEvent.SelectSmall;

      public AudioController.UIButtonAudioEvent SoundEvent {
        get { return _UISoundEvent; }
        set { _UISoundEvent = value; }
      }
      */

      protected override void OnEnable() {
        base.OnEnable();

        _ButtonView = GetComponent<AnkiWindow>();
        if(null == this.targetGraphic){
          this.targetGraphic = _ButtonView.BackgroundImage;
        }

        _HighlightedSprite = this.spriteState.highlightedSprite;
        _PressedSprite = this.spriteState.pressedSprite;
        _DisabledSprite = this.spriteState.disabledSprite;
        _TextLabel = _ButtonView.ContentPanel.GetComponentInChildren<Text>();
      }

      private void Tap() {
        if (!IsActive() || !IsInteractable()) {
          return;
        }
    
        PlayAudioEvent();
        _OnClick.Invoke();
      }

      private void Press() {
        if (!IsActive() || !IsInteractable()) {
          return;
        }

        _OnPress.Invoke();
      }

      private void Release() {
        if (!IsActive() || !IsInteractable()) {
          return;
        }
    
        _OnRelease.Invoke();
      }
  
      // Trigger all registered callbacks.
      public virtual void OnPointerClick(PointerEventData eventData) {
        if (eventData.button != PointerEventData.InputButton.Left) {
          return;
        }

        if (_TextLabel != null && _TextLabel.text.StartsWith("#")) {
          DAS.Event ("ui.button", _TextLabel.text, new Dictionary<string,string>{ { "$data", _DASEventName } });
        } else {
          DAS.Event ("ui.button", this.name , new Dictionary<string,string>{ { "$data", _DASEventName } });
        }

        Tap();
      }

      public override void OnPointerDown(PointerEventData eventData) {
        base.OnPointerDown(eventData);
        DAS.Debug("AnkiButton.OnPointerDown", string.Format("{0} Pressed", _DASEventName));

        Press();
      }

      public override void OnPointerUp(PointerEventData eventData) {
        base.OnPointerUp(eventData);
        DAS.Debug("AnkiButton.OnPointerUp", string.Format("{0} Released", _DASEventName));

        Release();
      }


      public void PlayAudioEvent() {
        // TODO: Audio
        //DAS.Debug("AnkiButton", string.Format("Playing {0} sound", _UISoundEvent));
        //AudioController.PostUIButtonAudioEvent(_UISoundEvent);
      }

    }
  }
}

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
      private ButtonClickedEvent _OnClick = new ButtonClickedEvent();

      [FormerlySerializedAs("onPress")]
      [SerializeField]
      private ButtonDownEvent _OnPress = new ButtonDownEvent();

      [FormerlySerializedAs("onRelease")]
      [SerializeField]
      private ButtonUpEvent _OnRelease = new ButtonUpEvent();

      [SerializeField]
      private string _DASEventName;

      [SerializeField]
      private Text _TextLabel;

      [SerializeField]
      private CanvasGroup _AlphaController;

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

      protected override void OnEnable() {
        base.OnEnable();

        if (_TextLabel == null) {
          _TextLabel = GetComponentInChildren<Text>();
        }
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
          DAS.Event("ui.button", _TextLabel.text, new Dictionary<string,string>{ { "$data", _DASEventName } });
        }
        else {
          DAS.Event("ui.button", this.name, new Dictionary<string,string>{ { "$data", _DASEventName } });
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

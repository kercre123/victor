using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Anki.Core.UI.Components;

namespace Cozmo.UI {
  public class CozmoButton : AnkiButton, IPointerClickHandler, IPointerDownHandler, IPointerUpHandler {

    private const int kViewControllerParentSearchLimit = 3;

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
    public new class ButtonClickedEvent : UnityEvent {
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

    private string _DASEventButtonName = "";

    private string _DASEventViewController = "";

    [SerializeField]
    private CozmoText _TextLabel;

    public object[] FormattingArgs {
      get {
        return _TextLabel.FormattingArgs;
      }
      set {
        _TextLabel.FormattingArgs = value;
      }
    }

    [SerializeField]
    private float _TextLabelOffset = 0f;
    private Vector3? _TextDefaultPosition = null;
    private Vector3 _TextPressedPosition;

    private bool _Initialized = false;

    public Color TextEnabledColor = Color.white;

    public Color TextPressedColor = Color.gray;

    public Color TextDisabledColor = Color.gray;

    [SerializeField]
    private CanvasGroup _AlphaController;

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

    public new ButtonClickedEvent onClick {
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

    public string DASEventButtonName {
      get {
        if (string.IsNullOrEmpty(_DASEventButtonName)) {
          _DASEventButtonName = this.gameObject.name;
        }
        return _DASEventButtonName;
      }
      set { _DASEventButtonName = value; }
    }

    public string DASEventViewController {
      get {
        // If no view controller is provided, use the names of this object's parents
        if (string.IsNullOrEmpty(_DASEventViewController)) {
          string[] parentNames = new string[kViewControllerParentSearchLimit];
          Transform current = this.transform;
          for (int i = 0; i < kViewControllerParentSearchLimit; i++) {
            if (current.parent != null) {
              current = current.parent;
              parentNames[i] = current.gameObject.name;
            }
          }
          System.Text.StringBuilder sb = new System.Text.StringBuilder();
          for (int i = kViewControllerParentSearchLimit - 1; i >= 0; i--) {
            if (!string.IsNullOrEmpty(parentNames[i])) {
              if (i != kViewControllerParentSearchLimit - 1) {
                sb.Append(".");
              }
              sb.Append(parentNames[i]);
            }
          }
          _DASEventViewController = sb.ToString();
          if (string.IsNullOrEmpty(_DASEventViewController)) {
            _DASEventViewController = "(parent is null)";
          }
        }
        return _DASEventViewController;
      }
      set { _DASEventViewController = value; }
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
    protected bool _OverrideButtonImageArray = false;
    public virtual bool OverrideButtonImageArray {
      get {
        return _OverrideButtonImageArray;
      }

      set {
        _OverrideButtonImageArray = value;
      }
    }


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

    protected virtual void HandleOnPress() {
      if (!_UISoundEvent.IsInvalid()) {
        Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_UISoundEvent);
      }
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

    public override bool IsInteractable() {
      return _Interactable;
    }

    private bool _IsInitialized = false;

    protected override void Start() {
      base.Start();
      if (!_Initialized && Application.isPlaying) {
        DAS.Warn("AnkiButton.Start", this.gameObject.name + " needs to be initialized.");
      }
      onClick.AddListener(HandleOnPress);

      UpdateVisuals();
    }

    protected override void OnDestroy() {
      base.OnDestroy();
      onClick.RemoveListener(HandleOnPress);
    }

    public void Initialize(UnityAction clickCallback, string dasEventButtonName, string dasEventViewController) {
      if (clickCallback != null) {
        onClick.AddListener(clickCallback);
      }
      _DASEventButtonName = dasEventButtonName;
      _DASEventViewController = dasEventViewController;

      if (string.IsNullOrEmpty(_DASEventButtonName)) {
        DAS.Error(this, string.Format("gameObject={0} is missing a DASButtonName! Falling back to gameObject name.",
          this.gameObject.name));
      }
      if (string.IsNullOrEmpty(_DASEventViewController)) {
        DAS.Error(this, string.Format("gameObject={0} is missing a DASViewController! Falling back to parent's names.",
          this.gameObject.name));
      }

      UpdateVisuals();
      _Initialized = true;
    }

    protected void InitializeDefaultGraphics() {
      if (!_IsInitialized && ButtonGraphics != null) {
        foreach (AnkiButtonImage graphic in ButtonGraphics) {
          if (graphic.targetImage != null) {
            graphic.targetImage.UpdateSkinnableElements(ThemesJson.GetCurrentTheme().Id, ThemesJson.GetCurrentThemeSkin().Id);
            graphic.enabledSprite = graphic.targetImage.sprite;
            graphic.enabledColor = graphic.targetImage.color;
          }
          else {
            DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name
                      + " targetImage=" + graphic.targetImage + " sprite="
                      + graphic.enabledSprite + ". Please check the prefab.");
          }
        }
        _IsInitialized = true;
      }
    }

    protected override void OnEnable() {
      base.OnEnable();

      // Grab the text label while in editor
      if (_TextLabel == null) {
        _TextLabel = GetComponentInChildren<CozmoText>();
      }
    }

    private bool DisableInteraction() {
      return !IsActive() || !IsInteractable();
    }

    private void Tap() {
      if (DisableInteraction()) {
        return;
      }

      StartCoroutine(DelayedResetButton());
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
      UpdateVisuals();
      _OnRelease.Invoke();
    }

    // Trigger all registered callbacks.
    public override void OnPointerClick(PointerEventData eventData) {
      if (eventData.button != PointerEventData.InputButton.Left) {
        return;
      }

      DAS.Event("ui.button", DASEventButtonName, new Dictionary<string, string> { { "$data", DASEventViewController } });

      Tap();
    }

    public override void OnPointerDown(PointerEventData eventData) {
      DAS.Debug("AnkiButton.OnPointerDown", string.Format("{0} Pressed - View: {1}", DASEventButtonName, DASEventViewController));
      Press();
    }

    public override void OnPointerUp(PointerEventData eventData) {
      DAS.Debug("AnkiButton.OnPointerUp", string.Format("{0} Released - View: {1}", DASEventButtonName, DASEventViewController));
      Release();
    }

    protected virtual void UpdateVisuals() {
      InitializeDefaultGraphics();
      if (_ShowDisabledStateWhenInteractable) {
        ShowDisabledState();
      }
      else {
        if (IsInteractable()) {
          ShowEnabledState();
        }
        else {
          ShowDisabledState();
        }
      }

    }

    protected virtual void ShowEnabledState() {
      if (_ShowDisabledStateWhenInteractable) {
        ShowDisabledState();
      }
      else {
        if (ButtonGraphics != null) {
          foreach (AnkiButtonImage graphic in ButtonGraphics) {
            if (graphic != null) {
              if (graphic.targetImage != null && graphic.enabledSprite != null) {
                SetGraphic(graphic, graphic.enabledSprite, graphic.enabledColor, graphic.ignoreSprite);
              }
              else {
                DAS.Error(this, "Found null graphic data in button! gameObject.name=" + gameObject.name
                          + " targetImage=" + graphic.targetImage + " sprite="
                          + graphic.enabledSprite + ". Have you initialized this button?");
              }
            }
            else {
              DAS.Error(this, "Found null graphic data in button! gameObject.name=" + gameObject.name +
                        ". Please check prefab hook-ups.");
            }
          }
        }

        ResetTextPosition(TextEnabledColor);
        ShowGlint(true);
      }

    }

    protected virtual void ShowPressedState() {
      if (_ShowDisabledStateWhenInteractable) {
        ShowDisabledState();
      }
      else {
        if (ButtonGraphics != null) {
          foreach (AnkiButtonImage graphic in ButtonGraphics) {
            if (graphic.targetImage != null && graphic.pressedSprite != null) {
              SetGraphic(graphic, graphic.pressedSprite, graphic.pressedColor, graphic.ignoreSprite);
            }
            else {
              DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name
                        + " targetImage=" + graphic.targetImage + " sprite="
                        + graphic.pressedSprite + ". Have you initialized this button?");
            }
          }

          PressTextPosition();
        }
        ShowGlint(true);
      }
    }

    protected virtual void ShowDisabledState() {
      if (ButtonGraphics != null) {
        foreach (AnkiButtonImage graphic in ButtonGraphics) {
          if (graphic.targetImage != null && graphic.disabledSprite != null) {
            SetGraphic(graphic, graphic.disabledSprite, graphic.disabledColor, graphic.ignoreSprite);
          }
          else {
            DAS.Error(this, "Found null graphic in button! gameObject.name=" + gameObject.name
                      + " targetImage=" + graphic.targetImage + " sprite="
                      + graphic.disabledSprite + ". Have you initialized this button?");
          }
        }

        ResetTextPosition(TextDisabledColor);
      }
      ShowGlint(false);
    }

    private void SetGraphic(AnkiButtonImage graphic, Sprite desiredSprite, Color desiredColor, bool ignoreSprite) {
      if (!ignoreSprite) {
        graphic.targetImage.sprite = desiredSprite ?? graphic.enabledSprite;
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

    public override void UpdateSkinnableElements(string themeId, string skinId) {
      if (!string.IsNullOrEmpty(_LinkedComponentId)) {
        base.UpdateSkinnableElements(themeId, skinId);
      }
      ThemesJson.ThemeComponentObj linkedComponentObj = ThemesJson.GetComponentById(themeId, skinId, _LinkedComponentId);
      if (linkedComponentObj != null && linkedComponentObj.SkinButtonImageArray && !OverrideButtonImageArray) {
        SkinButtonImageArray(linkedComponentObj);
      }

      if (linkedComponentObj != null && linkedComponentObj.SkinButtonTransition && !OverrideSkinTransition) {
        TextEnabledColor = linkedComponentObj.ButtonTransition.ColorBlock.normalColor;
        TextPressedColor = linkedComponentObj.ButtonTransition.ColorBlock.pressedColor;
        TextDisabledColor = linkedComponentObj.ButtonTransition.ColorBlock.disabledColor;
      }
    }

    private void SkinButtonImageArray(ThemesJson.ThemeComponentObj linkedComponentObj) {
      for (int i = 0; i < linkedComponentObj.ButtonImageArray.Length; ++i) {
        ButtonGraphics[i].pressedColor = linkedComponentObj.ButtonImageArray[i].PressedSpriteColor;
        if (!string.IsNullOrEmpty(linkedComponentObj.ButtonImageArray[i].PressedSpriteResourceKey)) {
          ButtonGraphics[i].pressedSprite = ThemeSystemUtils.sInstance.LoadSprite(linkedComponentObj.ButtonImageArray[i].PressedSpriteResourceKey);
        }

        ButtonGraphics[i].disabledColor = linkedComponentObj.ButtonImageArray[i].DisabledSpriteColor;
        if (!string.IsNullOrEmpty(linkedComponentObj.ButtonImageArray[i].DisabledSpriteResourceKey)) {
          ButtonGraphics[i].disabledSprite = ThemeSystemUtils.sInstance.LoadSprite(linkedComponentObj.ButtonImageArray[i].DisabledSpriteResourceKey);
        }

      }
    }

    [Serializable]
    public class AnkiButtonImage {
      public CozmoImage targetImage;
      public bool ignoreSprite = false;

      // we will just grab it from targetImage;
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

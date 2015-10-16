using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Slider that allows up to three AI assisted action requests via ActionButtons
///   one central button that by default engages the search and target lock features
///   optional upper and lower buttons that can be dragged to and then released to request two additional actions
/// </summary>
[System.Serializable]
public class ActionSlider {

  [System.Serializable]
  public class Hint {
    public Image image;
    public Text text;
  }

  public Slider slider;

  [SerializeField] private GameObject highlight;
  [SerializeField] private Text text;
  [SerializeField] private Image image;

  public bool Pressed { get; private set; }

  public ActionButton currentAction { get; private set; }

  public void SetMode(ActionButton button, bool down) {
    if (button == null) {
      DAS.Error("ActionSliderPanel", "Slider was given a current action of null");
      return;
    }

    currentAction = button;
    Pressed = down;

    if (button.mode != ActionButton.Mode.DISABLED) {
      highlight.SetActive(Pressed);
      text.gameObject.SetActive(Pressed);
    }
    
    text.text = currentAction.text.text;
    image.sprite = currentAction.image.sprite;

    if (button.hint.image != null) {
      button.hint.image.gameObject.SetActive(!Pressed && button.mode != ActionButton.Mode.DISABLED);
    }
  }

  public void SetHints() {
    if (ActionSliderPanel.instance != null) {
      for (int i = 0; i < ActionSliderPanel.instance.actionButtons.Length; ++i) {
        ActionButton button = ActionSliderPanel.instance.actionButtons[i];
        if (button.hint.image != null) {
          button.hint.image.gameObject.SetActive(!Pressed && i == 2); //button.mode != ActionButton.Mode.DISABLED);
        }
      }
    }
  }
}

/// <summary>
/// means by which our CozmoVision script manages the ActionSlider
/// </summary>
public class ActionSliderPanel : ActionPanel {
  public ActionSlider actionSlider;

  [SerializeField] private DynamicSliderFrame dynamicSliderFrame;
  [SerializeField] private AudioClip slideInSound;
  [SerializeField] private AudioClip slideOutSound;
  [SerializeField] private GameObject background;
  [SerializeField] private Animation actionAnimation;
  [SerializeField] private AnimationClip actionIdleAnim;
  [SerializeField] private AnimationClip actionSheenAnim;

  [SerializeField] private AudioClip actionsAvailableSound;
  [SerializeField] private AudioClip actionsNotAvailableSound;

  private ActionButton bottomAction { get { return actionButtons[0]; } }

  private ActionButton topAction { get { return actionButtons[1]; } }

  private ActionButton centerAction { get { return actionButtons[2]; } }

  public bool upLastFrame { get; private set; }

  public bool downLastFrame { get; private set; }

  private ActionButton.Mode lastMode = ActionButton.Mode.DISABLED;

  private bool secondaryActionsAvailableLastFrame = false;

  public static new ActionSliderPanel instance = null;

  protected override void Awake() {
    base.Awake();
    
    SetDefaults();
  }

  protected override void OnEnable() {
    base.OnEnable();

    instance = this;

    secondaryActionsAvailableLastFrame = false;

    if (actionAnimation != null) {
      actionAnimation.Stop();
      actionAnimation.Play(actionIdleAnim.name);
    }

  }

  protected override void OnDisable() {
    base.OnDisable();

    if (instance == this)
      instance = null;

    SetDefaults();

    AudioManager.Stop(actionsAvailableSound);
  }

  private void SetDefaults() {
    if (actionSlider != null) {
      actionSlider.slider.value = 0f;
      actionSlider.SetMode(centerAction, false);
    }
    
    upLastFrame = true;
    downLastFrame = false;
  }

  protected void Update() {
    lastMode = actionSlider.currentAction.mode;

    if (actionSlider != null)
      actionSlider.SetHints();
    
    if (robot == null || allDisabled) {
      if (dynamicSliderFrame != null)
        dynamicSliderFrame.enabled = false;
      upLastFrame = true;
      downLastFrame = false;

      RefreshSliderMode();
      return;
    }

    if (dynamicSliderFrame != null)
      dynamicSliderFrame.enabled = true;

    bool actionsAvailable = secondaryActionsAvailable;

    if (!actionsAvailable) {
      if (background != null)
        background.SetActive(false);
      actionSlider.slider.value = 0f;
    }
    else {
      if (background != null)
        background.SetActive(true);
    }

    if (!actionSlider.Pressed) {
      if (!upLastFrame)
        actionSlider.currentAction.OnRelease();

      upLastFrame = true;
      downLastFrame = false;

      if (actionAnimation != null && secondaryActionsAvailableLastFrame != actionsAvailable) {
        if (actionsAvailable) {
          actionAnimation.CrossFade(actionSheenAnim.name, 0.5f, PlayMode.StopAll);
        }
        else {
          actionAnimation.CrossFade(actionIdleAnim.name, 0.5f, PlayMode.StopAll);
        }
      }
      actionSlider.SetMode(centerAction, actionSlider.Pressed);
    }
    else {

      if (actionAnimation != null) {
        actionAnimation.Stop();
        actionAnimation.Play(actionIdleAnim.name);
      }

      if (!downLastFrame)
        actionSlider.currentAction.OnPress();

      downLastFrame = true;
      upLastFrame = false;

      RefreshSliderMode();
    }

    if (secondaryActionsAvailableLastFrame && !actionsAvailable) {
      AudioManager.PlayOneShot(actionsNotAvailableSound, 0f, 1f, 0.25f);
      AudioManager.Stop(actionsAvailableSound);
    }
    else if (!secondaryActionsAvailableLastFrame && actionsAvailable) {
      AudioManager.PlayAudioClipLooping(actionsAvailableSound, 0f, AudioManager.Source.Robot, 1f, 0.25f);
    }
    
    secondaryActionsAvailableLastFrame = actionsAvailable;
  }

  public void ToggleInteract(bool val) {
    if (!val) {
      actionSlider.slider.value = 0f;
      actionSlider.SetMode(actionSlider.currentAction, val);
    }
    else {
      actionSlider.SetMode(centerAction, val);
    }
  }

  private void RefreshSliderMode() {
    ActionButton currentButton = centerAction;

    if (actionSlider.slider.value < -0.5f && bottomAction.mode != ActionButton.Mode.DISABLED) {
      currentButton = bottomAction;

      float minZ = float.MaxValue;
      for (int i = 0; i < robot.seenObjects.Count && i < 2; i++) {
        if (minZ < robot.seenObjects[i].WorldPosition.z)
          continue;
        minZ = robot.seenObjects[i].WorldPosition.z;
        robot.targetLockedObject = robot.seenObjects[i];
      }
    }
    else if (actionSlider.slider.value > 0.5f && topAction.mode != ActionButton.Mode.DISABLED) {
      currentButton = topAction;
      
      float maxZ = float.MinValue;
      for (int i = 0; i < robot.seenObjects.Count && i < 2; i++) {
        if (maxZ > robot.seenObjects[i].WorldPosition.z)
          continue;
        maxZ = robot.seenObjects[i].WorldPosition.z;
        robot.targetLockedObject = robot.seenObjects[i];
      }
    }
    
    if (currentButton.mode != lastMode && currentButton != centerAction) {
      SlideInSound();
    }
    else if (currentButton.mode != lastMode) {
      SlideOutSound();
    }
    actionSlider.SetMode(currentButton, actionSlider.Pressed);
  }

  private void SlideInSound() {
    AudioManager.PlayOneShot(slideInSound);
  }

  private void SlideOutSound() {
    AudioManager.PlayOneShot(slideOutSound);
  }
}

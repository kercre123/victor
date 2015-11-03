using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;

/// <summary>
/// this class wraps the functionality of a particular on-screen input for requesting
///   an AI assisted robot action
/// 
///   originally designed to be flexible enough to handle a variety of different cozmoVision/action input schemes,
///     but now only really used by the ActionSliderPanel
/// </summary>
[System.Serializable]
public class ActionButton : MonoBehaviour {
  public enum Mode {
    DISABLED,
    TARGET,
    PICK_UP,
    DROP,
    STACK,
    ROLL,
    ALIGN,
    CHANGE,
    CANCEL,
    Count
  }

  [System.Serializable]
  public struct Hint {
    public Image image;
    public Text text;

    private Color _ghostText;

    public Color ghostText {
      get {
        if (_ghostText == Color.clear && text != null)
          _ghostText = text.color;
        return _ghostText;
      }
    }

    public Color solidText {
      get { 
        if (_ghostText == Color.clear && text != null)
          _ghostText = text.color;
        return Color.white;
      }
    }

    private Color _ghostImage;

    public Color ghostImage {
      get {
        if (_ghostImage == Color.clear && image != null)
          _ghostImage = image.color;
        return _ghostImage;
      }
    }

    public Color solidImage {
      get { 
        if (_ghostImage == Color.clear && image != null)
          _ghostImage = image.color;
        return Color.white;
      }
    }
  }

  [SerializeField] private Button button;

  public Hint hint;
  public Image image;
  public Text text;

  private string lastText;

  private string append;
  private string lastAppend;

  private bool solidHint;
  private bool lastSolidHint;

  private Action<bool, ObservedObject> action;

  public ObservedObject selectedObject { get; private set; }

  private ObservedObject lastSelectedObject;

  public Mode mode { get; private set; }

  private Mode lastMode;

  private Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  public bool changed { get { return lastMode != mode || lastSelectedObject != selectedObject || lastText != text.text; } }

  private void InvokeActions(bool released, ObservedObject manipulatedObject) {
    DefaultAction(released, manipulatedObject);
    if (action != null) {
      action(released, manipulatedObject);
    }
  }

  public void OnRelease() {
    InvokeActions(true, selectedObject);
  }

  public void OnPress() {
    InvokeActions(false, selectedObject);
  }

  public void SetLastMode() {
    lastMode = mode;
    lastSelectedObject = selectedObject;
    lastText = text.text;
    lastAppend = append;
    lastSolidHint = solidHint;
  }

  public void SetToLastMode() {
    SetMode(lastMode, lastSelectedObject, lastAppend, lastSolidHint);
  }

  public void SetMode(Mode mode, ObservedObject selectedObject, string append = null, bool solidHint = false) {
    GameActions gameActions = GameActions.instance;

    if (robot == null || gameActions == null) {
      mode = Mode.DISABLED;
    }

    action = null;
    this.mode = mode;
    this.selectedObject = selectedObject;
    this.append = append;
    this.solidHint = solidHint;

    if (mode == Mode.DISABLED) {
      if (button != null)
        button.gameObject.SetActive(false);
      if (hint.text != null)
        hint.text.gameObject.SetActive(false);
      gameObject.SetActive(false);
      return;
    }

    image.sprite = GetModeSprite(mode);
    text.text = GetModeName(mode) + append;

    if (hint.image != null) {
      if (hint.image.sprite != image.sprite)
        hint.image.sprite = image.sprite;

      if (solidHint) {
        hint.image.color = hint.solidImage;
      }
      else {
        hint.image.color = hint.ghostImage;
      }
    }

    if (hint.text != null) {
      if (hint.text.text != text.text)
        hint.text.text = text.text;

      hint.text.gameObject.SetActive(true/*solidHint*/);

//      if( solidHint )
//      {
//        hint.text.color = hint.solidText;
//      }
//      else
//      {
//        hint.text.color = hint.ghostText;
//      }
    }

    switch (mode) {
    case Mode.TARGET:
      action = gameActions.Target;
      break;
    case Mode.PICK_UP:
      action = gameActions.PickUp;
      break;
    case Mode.DROP:
      action = gameActions.Drop;
      break;
    case Mode.STACK:
      action = gameActions.Stack;
      break;
    case Mode.ROLL:
      action = gameActions.Roll;
      break;
    case Mode.ALIGN:
      action = gameActions.Align;
      break;
    case Mode.CHANGE:
      action = gameActions.Change;
      break;
    case Mode.CANCEL:
      action = gameActions.Cancel;
      break;
    }

    if (button != null)
      button.gameObject.SetActive(true);
    gameObject.SetActive(true);
  }

  private static Sprite GetModeSprite(Mode mode) {
    if (GameActions.instance != null) {
      switch (mode) {
      case Mode.TARGET:
        return GameActions.instance.GetActionSprite(mode);
      case Mode.PICK_UP:
        return GameActions.instance.GetActionSprite(mode);
      case Mode.DROP:
        return GameActions.instance.GetActionSprite(mode);
      case Mode.STACK:
        return GameActions.instance.GetActionSprite(mode);
      case Mode.ROLL:
        return GameActions.instance.GetActionSprite(mode);
      case Mode.ALIGN:
        return GameActions.instance.GetActionSprite(mode);
      case Mode.CHANGE:
        return GameActions.instance.GetActionSprite(mode);
      case Mode.CANCEL:
        return GameActions.instance.GetActionSprite(mode);
      }
    }
    
    return null;
  }

  private static string GetModeName(Mode mode) {
    if (GameActions.instance != null) {
      switch (mode) {
      case Mode.TARGET:
        return GameActions.instance.TARGET;
      case Mode.PICK_UP:
        return GameActions.instance.PICK_UP;
      case Mode.DROP:
        return GameActions.instance.DROP;
      case Mode.STACK:
        return GameActions.instance.STACK;
      case Mode.ROLL:
        return GameActions.instance.ROLL;
      case Mode.ALIGN:
        return GameActions.instance.ALIGN;
      case Mode.CHANGE:
        return GameActions.instance.CHANGE;
      case Mode.CANCEL:
        return GameActions.instance.CANCEL;
      }
    }
    
    return string.Empty;
  }

  private void DefaultAction(bool onRelease, ObservedObject selectedObject) {
    if (onRelease && robot != null) {
      robot.searching = false;
    }
  }
}

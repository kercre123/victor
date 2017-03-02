using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo.UI {
  public class CozmoRadioButton : CozmoButtonLegacy {

    [SerializeField]
    private Image _PressedTintImage;

    [SerializeField]
    private Color _PressedTintColor;

    public Color PressedTintColor {
      get { return _PressedTintColor; }
      set {
        if (value != _PressedTintColor) {
          _PressedTintColor = value;
          if (_PressedTintImage != null) {
            UpdatePressedColorOnImage(_PressedTintImage);
          }
        }
      }
    }

    [SerializeField]
    private bool _ShowPressedStateOnRelease = false;

    public bool ShowPressedStateOnRelease {
      get { return _ShowPressedStateOnRelease; }
      set {         _ShowPressedStateOnRelease = value;
        UpdateVisuals();
      }     }

    protected override void Start() {
      base.Start();
      if (_PressedTintImage != null) {
        UpdatePressedColorOnImage(_PressedTintImage);
      }
    }

    private void UpdatePressedColorOnImage(Image pressedTintImage) {
      foreach (AnkiButtonImage image in ButtonGraphics) {
        if (image.targetImage == pressedTintImage) {
          image.pressedColor = _PressedTintColor;
        }
      }
    }

    protected override void UpdateVisuals() {
      if (_ShowDisabledStateWhenInteractable) {
        InitializeDefaultGraphics();
        ShowDisabledState();
      }
      else if (_ShowPressedStateOnRelease) {
        InitializeDefaultGraphics();
        ShowPressedState();
      }
      else {
        base.UpdateVisuals();
      }
    }
  }
}
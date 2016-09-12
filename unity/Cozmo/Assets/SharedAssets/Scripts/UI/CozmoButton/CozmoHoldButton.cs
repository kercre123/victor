using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.UI {
  public class CozmoHoldButton : MonoBehaviour {

    [SerializeField]
    private CozmoButton _Button;

    [SerializeField]
    private Image _FillBarImage;

    [SerializeField]
    private float _SecondsToFillBar;

    public string Text {
      get { return _Button.Text; }
      set { _Button.Text = value; }
    }

    public bool Interactable {
      get { return _Button.Interactable; }
      set {
        _Button.Interactable = value;
        _FillBarImage.enabled = value;
      }
    }

    private float _FillSpeed;
    private System.Action _FillFinishedCallback;
    private bool _IsPressed = false;

    private void Start() {
      _FillBarImage.fillAmount = 0;
    }

    private void Update() {
      if (_IsPressed) {
        float targetProgress = _FillBarImage.fillAmount + Time.deltaTime * _FillSpeed;
        targetProgress = Mathf.Clamp(targetProgress, 0f, 1f);
        _FillBarImage.fillAmount = targetProgress;

        if (targetProgress >= 1) {
          if (_FillFinishedCallback != null) {
            _FillFinishedCallback();
          }

          _Button.onPress.RemoveListener(HandleButtonPressed);
          _Button.onRelease.RemoveListener(HandleButtonReleased);
          _IsPressed = false;
        }
      }
    }

    public void Initialize(System.Action fillFinishedCallback, string dasEventButtonName, string dasEventViewController,
                           float secondsToFillBar = -1f) {
      _Button.Initialize(null, dasEventButtonName, dasEventViewController);

      if (secondsToFillBar > 0) {
        _SecondsToFillBar = secondsToFillBar;
      }
      if (_SecondsToFillBar <= 0) {
        _SecondsToFillBar = 1f;
      }
      _FillSpeed = 1 / _SecondsToFillBar;

      _FillBarImage.fillAmount = 0;

      _FillFinishedCallback = fillFinishedCallback;

      _Button.onPress.AddListener(HandleButtonPressed);
      _Button.onRelease.AddListener(HandleButtonReleased);
    }

    public void ResetButton() {
      _FillBarImage.fillAmount = 0;
      _Button.onPress.RemoveAllListeners();
      _Button.onRelease.RemoveAllListeners();
      _Button.onPress.AddListener(HandleButtonPressed);
      _Button.onRelease.AddListener(HandleButtonReleased);
    }

    private void OnDestroy() {
      _Button.onPress.RemoveListener(HandleButtonPressed);
      _Button.onRelease.RemoveListener(HandleButtonReleased);
    }

    private void HandleButtonPressed() {
      _IsPressed = true;
    }

    private void HandleButtonReleased() {
      _FillBarImage.fillAmount = 0f;
      _IsPressed = false;
    }
  }
}
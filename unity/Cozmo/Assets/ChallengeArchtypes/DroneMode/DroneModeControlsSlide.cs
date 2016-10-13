using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeControlsSlide : MonoBehaviour {
    private const float _kSliderChangeThreshold = 0.01f;

    public delegate void SpeedSliderEventHandler(SpeedSliderSegment currentSegment, float newNormalizedSegmentValue);
    public event SpeedSliderEventHandler OnDriveSpeedSegmentValueChanged;

    public delegate void SpeedSliderSegmentEventHandler(SpeedSliderSegment currentSegment, SpeedSliderSegment newSegment);
    public event SpeedSliderSegmentEventHandler OnDriveSpeedSegmentChanged;

    public delegate void HeadSliderEventHandler(float newNormalizedSegmentValue);
    public event HeadSliderEventHandler OnHeadTiltSegmentValueChanged;

    public enum SpeedSliderSegment {
      Turbo,
      Forward,
      Neutral,
      Reverse
    }

    public bool ShowDebugTextFields = true;

    [SerializeField]
    private CozmoStickySlider _SpeedThrottle;

    [SerializeField, Range(0f, 1f)]
    private float _ReverseThreshold;

    [SerializeField, Range(0f, 1f)]
    private float _ForwardThreshold;

    [SerializeField, Range(0f, 1f)]
    private float _TurboThreshold;

    [SerializeField]
    private Slider _HeadTiltSlider;

    [SerializeField]
    private Color _BackgroundColor;
    public Color BackgroundColor { get { return _BackgroundColor; } }

    [SerializeField]
    private Sprite _QuitButtonSprite;
    public Sprite QuitButtonSprite { get { return _QuitButtonSprite; } }

    [SerializeField]
    private CozmoButton _HowToPlayButton;
    private DroneModeHowToPlayView _HowToPlayViewInstance;

    [SerializeField]
    private DroneModeHowToPlayView _HowToPlayViewPrefab;

    // TODO Remove debug text field
    public Anki.UI.AnkiTextLabel TiltText;

    // TODO Remove debug text field
    public Anki.UI.AnkiTextLabel DebugText;

    [SerializeField]
    private DroneModeCameraFeed _CameraFeed;

    private SpeedSliderSegment _CurrentDriveSpeedSliderSegment;
    private float _CurrentDriveSpeedSliderSegmentValue;

    private float _CurrentHeadSliderSegmentValue;

    private void Awake() {
      _HowToPlayButton.Initialize(HandleHowToPlayClicked, "drone_mode_how_to_play_button", "drone_mode_view_slide");
    }

    private void Start() {
      _CurrentDriveSpeedSliderSegment = SpeedSliderSegment.Neutral;
      _CurrentDriveSpeedSliderSegmentValue = 0f;
      _CurrentHeadSliderSegmentValue = 0f;
      _SpeedThrottle.onValueChanged.AddListener(HandleSpeedThrottleValueChanged);
      _HeadTiltSlider.onValueChanged.AddListener(HandleTiltThrottleValueChanged);

      TiltText.gameObject.SetActive(ShowDebugTextFields);
      DebugText.gameObject.SetActive(ShowDebugTextFields);
    }

    public void InitializeCameraFeed(IRobot currentRobot) {
      _CameraFeed.Initialize(currentRobot);
      _CameraFeed.DebugTextField.gameObject.SetActive(ShowDebugTextFields);
    }

    private void OnDestroy() {
      _SpeedThrottle.onValueChanged.RemoveListener(HandleSpeedThrottleValueChanged);
      _HeadTiltSlider.onValueChanged.RemoveListener(HandleTiltThrottleValueChanged);
      if (_HowToPlayViewInstance != null) {
        _HowToPlayViewInstance.CloseViewImmediately();
      }
    }

    private void HandleHowToPlayClicked() {
      if (UIManager.Instance.NumberOfOpenDialogues() == 0) {
        OpenHowToPlayView();
      }
    }

    public void OpenHowToPlayView() {
      if (_HowToPlayViewInstance == null) {
        _HowToPlayViewInstance = UIManager.OpenView<DroneModeHowToPlayView>(_HowToPlayViewPrefab);
        _SpeedThrottle.SetToRest();
      }
    }

    private void HandleSpeedThrottleValueChanged(float newSliderValue) {
      SpeedSliderSegment newSegment;
      float newSegmentValue;
      MapSpeedSliderValueToSegmentValue(newSliderValue, out newSegment, out newSegmentValue);

      if (!newSliderValue.IsNear(_CurrentDriveSpeedSliderSegmentValue, _kSliderChangeThreshold)
          || newSegment != _CurrentDriveSpeedSliderSegment) {
        _CurrentDriveSpeedSliderSegmentValue = newSegmentValue;
        if (OnDriveSpeedSegmentValueChanged != null) {
          OnDriveSpeedSegmentValueChanged(newSegment, newSegmentValue);
        }

        if (newSegment != _CurrentDriveSpeedSliderSegment) {
          if (OnDriveSpeedSegmentChanged != null) {
            OnDriveSpeedSegmentChanged(_CurrentDriveSpeedSliderSegment, newSegment);
          }
          _CurrentDriveSpeedSliderSegment = newSegment;
        }
      }
    }

    private void MapSpeedSliderValueToSegmentValue(float sliderValue, out SpeedSliderSegment newSegment, out float normalizedSegmentValue) {
      normalizedSegmentValue = 0;
      newSegment = SpeedSliderSegment.Neutral;
      if (sliderValue < _ReverseThreshold) {
        float difference = _ReverseThreshold - sliderValue;
        normalizedSegmentValue = difference / _ReverseThreshold;
        newSegment = SpeedSliderSegment.Reverse;
      }
      else if (sliderValue > _TurboThreshold) {
        normalizedSegmentValue = 1;
        newSegment = SpeedSliderSegment.Turbo;
      }
      else if (sliderValue > _ForwardThreshold) {
        float difference = sliderValue - _ForwardThreshold;
        normalizedSegmentValue = difference / (_TurboThreshold - _ForwardThreshold);
        newSegment = SpeedSliderSegment.Forward;
      }
    }

    private void HandleTiltThrottleValueChanged(float newSliderValue) {
      if (!newSliderValue.IsNear(_CurrentHeadSliderSegmentValue, _kSliderChangeThreshold)) {
        _CurrentHeadSliderSegmentValue = newSliderValue;
        if (OnHeadTiltSegmentValueChanged != null) {
          OnHeadTiltSegmentValueChanged(newSliderValue);
        }
      }
    }
  }
}
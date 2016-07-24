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

    public delegate void HeadSliderEventHandler(HeadSliderSegment currentSegment, float newNormalizedSegmentValue);
    public event HeadSliderEventHandler OnHeadTiltSegmentValueChanged;
    public event HeadSliderEventHandler OnHeadTiltSegmentChanged;

    public enum SpeedSliderSegment {
      Turbo,
      Forward,
      Neutral,
      Reverse
    }

    public enum HeadSliderSegment {
      Forward,
      Neutral,
      Reverse
    }

    [SerializeField]
    private RawImage _CameraFeedImage;
    private ImageReceiver _ImageProcessor;

    [SerializeField]
    private RectTransform _CameraFeedImageContainer;
    private bool _IsCameraSized;

    [SerializeField]
    private CozmoStickySlider _SpeedThrottle;

    [SerializeField, Range(0f, 1f)]
    private float _ReverseThreshold;

    [SerializeField, Range(0f, 1f)]
    private float _ForwardThreshold;

    [SerializeField, Range(0f, 1f)]
    private float _TurboThreshold;

    [SerializeField]
    private CozmoStickySlider _HeadTiltThrottle;

    [SerializeField, Range(0f, 1f)]
    private float _HeadDownThreshold;

    [SerializeField, Range(0f, 1f)]
    private float _HeadUpThreshold;

    // TODO remove
    public Anki.UI.AnkiTextLabel TiltText;

    private SpeedSliderSegment _CurrentDriveSpeedSliderSegment;
    private float _CurrentDriveSpeedSliderSegmentValue;

    private HeadSliderSegment _CurrentHeadTiltSliderSegment;
    private float _CurrentHeadSliderSegmentValue;
    private IRobot _CurrentRobot;

    private void Start() {
      _CurrentDriveSpeedSliderSegment = SpeedSliderSegment.Neutral;
      _CurrentDriveSpeedSliderSegmentValue = 0f;
      _CurrentHeadTiltSliderSegment = HeadSliderSegment.Neutral;
      _CurrentHeadSliderSegmentValue = 0f;
      _SpeedThrottle.onValueChanged.AddListener(HandleSpeedThrottleValueChanged);
      _HeadTiltThrottle.onValueChanged.AddListener(HandleTiltThrottleValueChanged);
    }

    public void InitializeCameraFeed(IRobot currentRobot) {
      _ImageProcessor = new ImageReceiver("DroneModeCameraFeed");
      _ImageProcessor.OnImageSizeChanged += HandleImageSizeChanged;
      _ImageProcessor.Initialize(Anki.Cozmo.ImageSendMode.Off);
      _CameraFeedImage.texture = _ImageProcessor.Image;
      _CurrentRobot = currentRobot;
    }

    private void OnDestroy() {
      _SpeedThrottle.onValueChanged.RemoveListener(HandleSpeedThrottleValueChanged);
      _HeadTiltThrottle.onValueChanged.RemoveListener(HandleTiltThrottleValueChanged);

      _ImageProcessor.OnImageSizeChanged -= HandleImageSizeChanged;
      _ImageProcessor.Dispose();
      _ImageProcessor.DestroyTexture();
    }

    private void HandleImageSizeChanged(float width, float height) {
      _CameraFeedImage.rectTransform.sizeDelta = new Vector2(width, height);
      float imageScale = (_CameraFeedImageContainer.sizeDelta.x / width);
      _CameraFeedImage.rectTransform.localScale = new Vector3(imageScale, imageScale, imageScale);
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
      HeadSliderSegment newSegment;
      float newSegmentValue;
      MapHeadSliderValueToSegmentValue(newSliderValue, out newSegment, out newSegmentValue);

      if (!newSliderValue.IsNear(_CurrentHeadSliderSegmentValue, _kSliderChangeThreshold)
          || newSegment != _CurrentHeadTiltSliderSegment) {
        _CurrentHeadSliderSegmentValue = newSegmentValue;
        if (OnHeadTiltSegmentValueChanged != null) {
          OnHeadTiltSegmentValueChanged(newSegment, newSegmentValue);
        }

        if (newSegment != _CurrentHeadTiltSliderSegment) {
          _CurrentHeadTiltSliderSegment = newSegment;
          if (OnHeadTiltSegmentChanged != null) {
            OnHeadTiltSegmentChanged(newSegment, newSegmentValue);
          }
        }
      }
    }

    private void MapHeadSliderValueToSegmentValue(float sliderValue, out HeadSliderSegment newSegment, out float normalizedSegmentValue) {
      normalizedSegmentValue = 0;
      newSegment = HeadSliderSegment.Neutral;
      if (sliderValue < _HeadDownThreshold) {
        float difference = _HeadDownThreshold - sliderValue;
        normalizedSegmentValue = difference / _HeadDownThreshold;
        newSegment = HeadSliderSegment.Reverse;
      }
      else if (sliderValue > _HeadUpThreshold) {
        float difference = sliderValue - _HeadUpThreshold;
        normalizedSegmentValue = difference / (1 - _HeadUpThreshold);
        newSegment = HeadSliderSegment.Forward;
      }
    }
  }
}
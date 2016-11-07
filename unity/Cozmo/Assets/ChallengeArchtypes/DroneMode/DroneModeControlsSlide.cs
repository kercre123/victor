using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using System.Collections.Generic;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeControlsSlide : MonoBehaviour {
    private const string _kDasViewControllerName = "drone_mode_view_slide";
    private const float _kSliderChangeThreshold = 0.01f;

    public delegate void SpeedSliderEventHandler(SpeedSliderSegment currentSegment, float newNormalizedSegmentValue);
    public event SpeedSliderEventHandler OnDriveSpeedSegmentValueChanged;

    public delegate void SpeedSliderSegmentEventHandler(SpeedSliderSegment currentSegment, SpeedSliderSegment newSegment);
    public event SpeedSliderSegmentEventHandler OnDriveSpeedSegmentChanged;

    public delegate void HeadSliderEventHandler(float newValue);
    public event HeadSliderEventHandler OnHeadSliderValueChanged;

    public delegate void LiftSliderEventHandler(float newValue);
    public event LiftSliderEventHandler OnLiftSliderValueChanged;

    public enum SpeedSliderSegment {
      Turbo,
      Forward,
      Neutral,
      Reverse
    }

    // Can this be configurable?
    public enum ActionContextType {
      CubeInLift,
      CubeNotInLift,
      FaceSeen
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
    private Slider _LiftSlider;

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

    [SerializeField]
    private CozmoToggleButton _NightVisionButton;

    [SerializeField]
    private CozmoToggleButton _HeadLiftToggleButton;

    [SerializeField]
    private DroneModeCameraFeed _CameraFeed;

    [SerializeField]
    private GameObject _FaceButtonContainer;

    [SerializeField]
    private GameObject _CubeInLiftButtonContainer;

    [SerializeField]
    private GameObject _CubeNotInLiftButtonContainer;

    [SerializeField]
    private DroneModeActionButton _DroneModeActionButtonPrefab;

    private SpeedSliderSegment _CurrentDriveSpeedSliderSegment;
    private float _CurrentDriveSpeedSliderSegmentValue;

    private float _CurrentHeadSliderValue;

    private float _CurrentLiftSliderValue;

    private IVisibleInCamera _CurrentlyFocusedObject = null;
    public IVisibleInCamera CurrentlyFocusedObject { get { return _CurrentlyFocusedObject; } }

    private bool _IsCubeInLift = false;
    private IRobot _CurrentRobot;
    private List<DroneModeActionButton> _ContextualButtons;
    private bool _UpdateContextMenuBasedOnCurrentFocus = true;

    // TODO Remove debug text field
    public Anki.UI.AnkiTextLabel TiltText;

    // TODO Remove debug text field
    public Anki.UI.AnkiTextLabel DebugText;

    private void Awake() {
      _HowToPlayButton.Initialize(HandleHowToPlayClicked, "drone_mode_how_to_play_button", _kDasViewControllerName);
      _NightVisionButton.Initialize(HandleNightVisionButtonClicked, "night_vision_toggle_button", _kDasViewControllerName);
      _HeadLiftToggleButton.Initialize(null, "head_lift_toggle_button", _kDasViewControllerName);
      _ContextualButtons = new List<DroneModeActionButton>();
    }

    private void Start() {
      _CurrentDriveSpeedSliderSegment = SpeedSliderSegment.Neutral;
      _CurrentDriveSpeedSliderSegmentValue = 0f;
      _SpeedThrottle.onValueChanged.AddListener(HandleSpeedThrottleValueChanged);

      _CurrentHeadSliderValue = 0f;
      _HeadTiltSlider.value = _CurrentHeadSliderValue;
      _HeadTiltSlider.onValueChanged.AddListener(HandleHeadSliderValueChanged);

      TiltText.gameObject.SetActive(ShowDebugTextFields);
      DebugText.gameObject.SetActive(ShowDebugTextFields);
    }

    public void InitializeCameraFeed(IRobot currentRobot) {
      _CameraFeed.Initialize(currentRobot);
      _CameraFeed.DebugTextField.gameObject.SetActive(ShowDebugTextFields);

      _CameraFeed.OnCurrentFocusChanged += HandleCurrentFocusedObjectChanged;
      _CurrentlyFocusedObject = _CameraFeed.CurrentlyFocusedObject;

      _CurrentRobot = currentRobot;
      _CurrentRobot.OnCarryingObjectSet += HandleRobotCarryingObjectSet;
      _IsCubeInLift = (_CurrentRobot.CarryingObject != null);

      UpdateContextMenu();
    }

    public void InitializeLiftSlider(float sliderValue) {
      _CurrentLiftSliderValue = sliderValue;
      _LiftSlider.value = _CurrentLiftSliderValue;
      _LiftSlider.onValueChanged.AddListener(HandleLiftSliderValueChanged);
    }

    public void CreateActionButton(DroneModeActionData actionData, System.Action callback,
                                   bool interactableOnlyWhenCubeSeen, bool interactableOnlyWhenKnownFaceSeen,
                                   string dasButtonName, ActionContextType actionType) {
      GameObject buttonContainer = GetContainerBasedOnActionType(actionType);
      GameObject newButton = UIManager.CreateUIElement(_DroneModeActionButtonPrefab, buttonContainer.transform);
      DroneModeActionButton newButtonScript = newButton.GetComponent<DroneModeActionButton>();
      newButtonScript.Initialize(dasButtonName, _kDasViewControllerName, actionData, callback, interactableOnlyWhenCubeSeen, interactableOnlyWhenKnownFaceSeen);
      _ContextualButtons.Add(newButtonScript);
    }

    private GameObject GetContainerBasedOnActionType(ActionContextType actionType) {
      GameObject container = null;
      switch (actionType) {
      case ActionContextType.CubeInLift:
        container = _CubeInLiftButtonContainer;
        break;
      case ActionContextType.CubeNotInLift:
        container = _CubeNotInLiftButtonContainer;
        break;
      case ActionContextType.FaceSeen:
        container = _FaceButtonContainer;
        break;
      default:
        DAS.Error("DroneModeControlsSlide.GetContainerBasedOnActionType.ActionContextTypeNotSupported",
                  "Could not find container for actionType=" + actionType);
        break;
      }
      return container;
    }

    private void OnDestroy() {
      _SpeedThrottle.onValueChanged.RemoveListener(HandleSpeedThrottleValueChanged);
      _HeadTiltSlider.onValueChanged.RemoveListener(HandleHeadSliderValueChanged);
      _LiftSlider.onValueChanged.RemoveListener(HandleLiftSliderValueChanged);
      if (_HowToPlayViewInstance != null) {
        _HowToPlayViewInstance.CloseViewImmediately();
      }

      _CameraFeed.OnCurrentFocusChanged -= HandleCurrentFocusedObjectChanged;
      if (_CurrentRobot != null) {
        _CurrentRobot.OnCarryingObjectSet -= HandleRobotCarryingObjectSet;
        _CurrentRobot.SetNightVision(false);
      }
    }

    public void EnableInput() {
      EnableHeadSlider();
      _LiftSlider.interactable = true;
      _SpeedThrottle.interactable = true;
      UpdateContextualButtons();

      _UpdateContextMenuBasedOnCurrentFocus = true;
      UpdateContextMenu();
    }

    public void DisableInput() {
      DisableHeadSlider();
      _LiftSlider.interactable = false;
      _SpeedThrottle.interactable = false;
      foreach (DroneModeActionButton button in _ContextualButtons) {
        button.Interactable = false;
      }
      _UpdateContextMenuBasedOnCurrentFocus = false;
    }

    public void EnableHeadSlider() {
      _HeadTiltSlider.interactable = true;
    }

    public void DisableHeadSlider() {
      _HeadTiltSlider.interactable = false;
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

    private void HandleHeadSliderValueChanged(float newSliderValue) {
      if (!newSliderValue.IsNear(_CurrentHeadSliderValue, _kSliderChangeThreshold)) {
        _CurrentHeadSliderValue = newSliderValue;
        if (OnHeadSliderValueChanged != null) {
          OnHeadSliderValueChanged(newSliderValue);
        }
      }
    }

    private void HandleLiftSliderValueChanged(float newSliderValue) {
      if (!newSliderValue.IsNear(_CurrentLiftSliderValue, _kSliderChangeThreshold)) {
        _CurrentLiftSliderValue = newSliderValue;
        if (OnLiftSliderValueChanged != null) {
          OnLiftSliderValueChanged(newSliderValue);
        }
      }
    }

    private void HandleCurrentFocusedObjectChanged(IVisibleInCamera newObjectInView) {
      if (newObjectInView != _CurrentlyFocusedObject) {
        _CurrentlyFocusedObject = newObjectInView;

        if (_UpdateContextMenuBasedOnCurrentFocus) {
          UpdateContextMenu();
        }
      }
    }

    private void HandleRobotCarryingObjectSet(ObservedObject newObjectInLift) {
      bool isCubeInLift = (newObjectInLift != null && (newObjectInLift is LightCube));
      if (_IsCubeInLift != isCubeInLift) {
        _IsCubeInLift = isCubeInLift;
        UpdateContextMenu();
      }
    }

    private void UpdateContextMenu() {
      // Hide all containers
      _FaceButtonContainer.SetActive(false);
      _CubeInLiftButtonContainer.SetActive(false);
      _CubeNotInLiftButtonContainer.SetActive(false);

      bool anyContainerShown = false;
      if (_CurrentlyFocusedObject != null) {
        if (_CurrentlyFocusedObject is Face) {
          // Show face container
          _FaceButtonContainer.SetActive(true);
          anyContainerShown = true;
        }
        else if (_IsCubeInLift) {
          //   Show cube in lift container
          _CubeInLiftButtonContainer.SetActive(true);
          anyContainerShown = true;
        }
        else if (_CurrentlyFocusedObject is LightCube) {
          // Show cube seen container
          _CubeNotInLiftButtonContainer.SetActive(true);
          anyContainerShown = true;
        }
      }
      else if (_IsCubeInLift) {
        //   Show cube in lift container
        _CubeInLiftButtonContainer.SetActive(true);
        anyContainerShown = true;
      }

      if (anyContainerShown) {
        UpdateContextualButtons();
      }
    }

    private void UpdateContextualButtons() {
      bool currentObjectIsCube = (_CurrentlyFocusedObject is LightCube);
      bool currentObjectIsKnownFace = ((_CurrentlyFocusedObject is Face)
                                       && (!string.IsNullOrEmpty(((Face)_CurrentlyFocusedObject).Name)));
      foreach (DroneModeActionButton button in _ContextualButtons) {
        if (button.IsUnlocked) {
          if (button.NeedsCubeSeen) {
            button.Interactable = currentObjectIsCube;
          }
          else if (button.NeedsKnownFaceSeen) {
            button.Interactable = currentObjectIsKnownFace;
          }
          else {
            button.Interactable = true;
          }
        }
        else {
          button.Interactable = true;
        }
      }
    }

    public void SetHeadSliderValue(float headValue_deg) {
      _HeadTiltSlider.value = headValue_deg;
    }

    public void SetLiftSliderValue(float liftFactor) {
      _LiftSlider.value = liftFactor;
    }

    public void HandleNightVisionButtonClicked() {
      _CurrentRobot.SetNightVision(_NightVisionButton.IsCurrentlyOn);
    }
  }
}
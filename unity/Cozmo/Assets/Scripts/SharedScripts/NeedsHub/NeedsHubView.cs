using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Needs.UI {
  public class NeedsHubView : BaseView {
    private const int kCubesCount = 3;

    public delegate void StartMinigameClickedHandler();
    public event StartMinigameClickedHandler OnStartMinigameClicked;

    [SerializeField]
    private CozmoButton _PlayRandomMinigameButton;

    [SerializeField]
    private CozmoButton _SettingsButton;

    [SerializeField]
    private NeedsSettingsWidget _SettingsWidget;

    private bool _SettingsIsOpen = false;

    public void Start() {
      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe,
                                                                      Color.gray);
      _PlayRandomMinigameButton.Initialize(HandlePlayMinigameButtonClicked, "play_random_minigame_button", DASEventDialogName);
      _SettingsButton.Initialize(HandleSettingsButton, "settings_button", DASEventDialogName);
    }

    private void HandlePlayMinigameButtonClicked() {
      if (OnStartMinigameClicked != null) {
        OnStartMinigameClicked();
      }
    }

    private void HandleSettingsButton() {
      if (!_SettingsIsOpen) {
        _PlayRandomMinigameButton.gameObject.SetActive(false);
        ShowSettings();
        _SettingsIsOpen = true;
      }
      else {
        _PlayRandomMinigameButton.gameObject.SetActive(true);
        _SettingsIsOpen = false;
        _SettingsWidget.HideSettings();
      }
    }

    private void ShowSettings() {
      _SettingsWidget.ShowSettings();

      // auto scroll to the cubes setting panel if we don't have three cubes connected.
      if (RobotEngineManager.Instance.CurrentRobot != null && RobotEngineManager.Instance.CurrentRobot.LightCubes.Count != kCubesCount) {
        _SettingsWidget.ScrollToCubeSettings();
      }
    }
  }
}
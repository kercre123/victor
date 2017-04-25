using Cozmo.UI;
using DG.Tweening;
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

    [SerializeField]
    private NeedsMetersWidget _MetersWidget;

    // IVY TODO: Add game data for Feed
    // IVY TODO: Add game data for Repair
    // IVY TODO: Add play modal for Play

    public void Start() {
      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe,
                                                                      Color.gray);
      _PlayRandomMinigameButton.Initialize(HandlePlayMinigameButtonClicked, "play_random_minigame_button", DASEventDialogName);
      _SettingsButton.Initialize(HandleSettingsButton, "settings_button", DASEventDialogName);

      // IVY TODO: Listen for button click events for Feed, Repair, and Play modal/game handling
      _MetersWidget.Initialize(allowButtonInput: true,
                               enableButtonBasedOnNeeds: true,
                               dasParentDialogName: DASEventDialogName,
                               baseDialog: this);
    }

    protected override void CleanUp() {

    }

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      ConstructDefaultFadeOpenAnimation(openAnimation);
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      ConstructDefaultFadeCloseAnimation(closeAnimation);
    }

    private void HandlePlayMinigameButtonClicked() {
      if (OnStartMinigameClicked != null) {
        OnStartMinigameClicked();
      }
    }

    private void HandleSettingsButton() {
      if (!_SettingsIsOpen) {
        _MetersWidget.gameObject.SetActive(false);
        _PlayRandomMinigameButton.gameObject.SetActive(false);
        ShowSettings();
        _SettingsIsOpen = true;
      }
      else {
        _MetersWidget.gameObject.SetActive(true);
        _PlayRandomMinigameButton.gameObject.SetActive(true);
        _SettingsWidget.HideSettings();
        _SettingsIsOpen = false;
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
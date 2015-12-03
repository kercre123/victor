using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using System.Collections;

// Provides common interface for HubWorlds to react to games
// ending and to start/restart games. Also has interface for killing games
public abstract class GameBase : MonoBehaviour {

  public delegate void MiniGameQuitHandler();

  public event MiniGameQuitHandler OnMiniGameQuit;

  protected void RaiseMiniGameQuit() {
    if (OnMiniGameQuit != null) {
      OnMiniGameQuit();
    }
  }

  public delegate void MiniGameWinHandler();

  public event MiniGameWinHandler OnMiniGameWin;

  public void RaiseMiniGameWin() {
    if (OnMiniGameWin != null) {
      OnMiniGameWin();
    }
  }

  public delegate void MiniGameLoseHandler();

  public event MiniGameWinHandler OnMiniGameLose;

  protected void RaiseMiniGameLose() {
    if (OnMiniGameLose != null) {
      OnMiniGameLose();
    }
  }

  public Robot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private Button _QuitButtonInstance;

  public abstract void LoadMinigameConfig(MinigameConfigBase minigameConfigData);

  public void OnDestroy() {
    CleanUpOnDestroy();
  }

  /// <summary>
  /// Clean up listeners and extra game objects. Called before the game is 
  /// destroyed when the player quits or the robot loses connection.
  /// </summary>
  protected abstract void CleanUpOnDestroy();

  protected virtual void PauseGame() {
    // Disable quit button
    if (_QuitButtonInstance != null) {
      _QuitButtonInstance.interactable = false;
    }
  }

  protected virtual void ResumeGame() {
    // Enable quit button
    if (_QuitButtonInstance != null) {
      _QuitButtonInstance.interactable = true;
    }
  }

  public void CloseMinigameView() {
    // TODO: Play an animation on the quit button if it exists
    // TODO: Play an animation on the other UI if it exists
    Destroy(gameObject);
  }

  public void CloseMinigameViewImmediately() {
    DestroyDefaultQuitButton();
    Destroy(gameObject);
  }

  #region Default Quit button

  protected void CreateDefaultQuitButton() {
    GameObject newButton = UIManager.CreateUIElement(UIPrefabHolder.Instance.DefaultQuitGameButtonPrefab);
    // TODO: use ankibutton
    _QuitButtonInstance = newButton.GetComponent<Button>();
    _QuitButtonInstance.onClick.AddListener(HandleQuitButtonTap);
  }

  private void DestroyDefaultQuitButton() {
    if (_QuitButtonInstance != null) {
      _QuitButtonInstance.onClick.RemoveAllListeners();
      Destroy(_QuitButtonInstance.gameObject);
    }
  }

  private void HandleQuitButtonTap() {
    // Open confirmation dialog instead
    SimpleAlertView alertView = UIManager.OpenView(UIPrefabHolder.Instance.AlertViewPrefab) as SimpleAlertView;
    // Hook up callbacks
    alertView.SetCloseButtonEnabled(true);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleQuitConfirmed);
    alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleQuitCancelled);
    alertView.TitleLocKey = LocalizationKeys.kMinigameQuitViewTitle;
    alertView.DescriptionLocKey = LocalizationKeys.kMinigameQuitViewDescription;
    // Listen for dialog close
    alertView.ViewCloseAnimationFinished += HandleQuitViewClosed;
    PauseGame();
  }

  private void HandleQuitCancelled() {
    // Do nothing; we'll resume when the dialog closes.
  }

  private void HandleQuitConfirmed() {
    RaiseMiniGameQuit();
  }

  private void HandleQuitViewClosed() {
    ResumeGame();
  }

  #endregion

  #region Default Stamina Bar

  private void CreateDefaultStaminaBar() {
  }

  #endregion
}
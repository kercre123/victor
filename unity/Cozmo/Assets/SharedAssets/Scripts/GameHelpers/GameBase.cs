using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
using Cozmo.MinigameWidgets;
using DG.Tweening;

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

  public abstract void LoadMinigameConfig(MinigameConfigBase minigameConfigData);

  protected SharedMinigameView _SharedMinigameViewInstance;

  public void Awake() {
    // INGO: We might need to have some sort of callback when the 
    // dialog is initialized
    OpenMinigame();
  }

  public void OnDestroy() {
    CleanUpOnDestroy();
  }

  /// <summary>
  /// Clean up listeners and extra game objects. Called before the game is 
  /// destroyed when the player quits or the robot loses connection.
  /// </summary>
  protected abstract void CleanUpOnDestroy();

  protected virtual void PauseGame() {
    _SharedMinigameViewInstance.DisableInteractivity();
  }

  protected virtual void ResumeGame() {
    _SharedMinigameViewInstance.EnableInteractivity();
  }

  private void OpenMinigame() {
    _SharedMinigameViewInstance = UIManager.OpenView(UIPrefabHolder.Instance.SharedMinigameViewPrefab) as SharedMinigameView;
  }

  public void CloseMinigame() {
    _SharedMinigameViewInstance.CloseView();
    _SharedMinigameViewInstance = null;
  }

  public void CloseMinigameImmediately() {
    if (_SharedMinigameViewInstance != null) {
      _SharedMinigameViewInstance.CloseViewImmediately();
      _SharedMinigameViewInstance = null;
    }
    Destroy(gameObject);
  }

  #region Default Quit button

  protected void CreateDefaultQuitButton() {
    _SharedMinigameViewInstance.CreateQuitButton();
    _SharedMinigameViewInstance.QuitMiniGameViewOpened += HandleQuitViewOpened;
    _SharedMinigameViewInstance.QuitMiniGameViewClosed += HandleQuitViewClosed;
    _SharedMinigameViewInstance.QuitMiniGameConfirmed += HandleQuitConfirmed;
  }

  private void HandleQuitViewOpened() {
    PauseGame();
  }

  private void HandleQuitViewClosed() {
    ResumeGame();
  }

  private void HandleQuitConfirmed() {
    RaiseMiniGameQuit();
  }

  #endregion

  #region Default Stamina Bar

  private void CreateDefaultStaminaBar() {
  }

  #endregion
}
using UnityEngine;
using UnityEngine.UI;
using System.Collections;

// Provides common interface for HubWorlds to react to games
// ending and to start/restart games. Also has interface for killing games
public abstract class GameBase : MonoBehaviour {

  private static GameObject sDefaultQuitGameButtonPrefab;

  public delegate void MiniGameQuitHandler();

  public event MiniGameQuitHandler OnMiniGameQuit;

  protected void RaiseMiniGameQuit() {
    if (OnMiniGameQuit != null) {
      OnMiniGameQuit();
    }
  }

  private Button _QuitButtonInstance;
  
  [SerializeField]
  string _GameId;

  public string GameId { get { return _GameId; } private set { _GameId = value; } }

  public Robot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  protected void CreateDefaultQuitButton() {
    // Resources.Load can be pretty slow, so cache the prefab for future use.
    if (sDefaultQuitGameButtonPrefab == null) {
      sDefaultQuitGameButtonPrefab = Resources.Load("Prefabs/UI/DefaultQuitMiniGameButton") as GameObject;
    }
    GameObject newButton = UIManager.CreateUI(sDefaultQuitGameButtonPrefab);
    
    _QuitButtonInstance = newButton.GetComponent<Button>();
    _QuitButtonInstance.onClick.AddListener(OnQuitButtonTap);
  }

  protected void DestroyDefaultQuitButton() {
    if (_QuitButtonInstance != null) {
      Destroy(_QuitButtonInstance.gameObject);
    }
  }

  protected void OnQuitButtonTap() {
    RaiseMiniGameQuit();
  }

  /// <summary>
  /// Clean up listeners and extra game objects. Called before the game is 
  /// destroyed when the player quits or the robot loses connection.
  /// </summary>
  public abstract void CleanUp();
}
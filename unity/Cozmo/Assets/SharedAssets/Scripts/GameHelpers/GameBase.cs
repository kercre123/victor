using UnityEngine;
using UnityEngine.UI;
using System.Collections;

// Provides common interface for HubWorlds to react to games
// ending and to start/restart games. Also has interface for killing games
public abstract class GameBase : MonoBehaviour {

  private static GameObject s_defaultQuitGameButtonPrefab;

  public delegate void MiniGameQuitHandler();
  public event MiniGameQuitHandler OnMiniGameQuit;
  protected void RaiseMiniGameQuit(){
    if (OnMiniGameQuit != null) {
      OnMiniGameQuit();
    }
  }

  private Button quitButtonInstance_;
  
  [SerializeField]
  string gameName_;
  public string GameName { get { return gameName_; } private set { gameName_ = value; } }

  public Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.CurrentRobot : null; } }
  
  protected void CreateDefaultQuitButton() {
    // Resources.Load can be pretty slow, so cache the prefab for future use.
    if (s_defaultQuitGameButtonPrefab == null) {
      s_defaultQuitGameButtonPrefab = Resources.Load("Prefabs/UI/DefaultQuitMiniGameButton") as GameObject;
    }
    GameObject newButton = UIManager.CreateUI (s_defaultQuitGameButtonPrefab);
    
    quitButtonInstance_ = newButton.GetComponent<Button> ();
    quitButtonInstance_.onClick.AddListener (OnQuitButtonTap);
  }

  protected void DestroyDefaultQuitButton() {
    if (quitButtonInstance_ != null) {
      Destroy(quitButtonInstance_.gameObject);
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
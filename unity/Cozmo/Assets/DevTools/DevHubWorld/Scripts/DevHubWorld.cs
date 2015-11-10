using UnityEngine;
using System.Collections;

public class DevHubWorld : HubWorldBase {

  [SerializeField]
  private GameBase[] _GamePrefabs;
 
  [SerializeField]
  private DevHubWorldDialog _DevHubWorldDialogPrefab;
  private DevHubWorldDialog _DevHubWorldDialogInstance;

  private GameBase _MiniGameInstance;

  public override bool LoadHubWorld() {
    ShowHubWorldDialog();
    return true;
  }

  public override bool DestroyHubWorld() {
    
    // Deregister events
    // Destroy dialog if it exists
    if (_DevHubWorldDialogInstance != null) {
      _DevHubWorldDialogInstance.OnDevButtonClicked -= OnDevButtonClicked;
      _DevHubWorldDialogInstance.CloseDialogImmediately();
    }
    
    CloseMiniGame();
    return true;
  }

  private void ShowHubWorldDialog() {
    // Create dialog with the game prefabs
    _DevHubWorldDialogInstance = UIManager.OpenDialog(_DevHubWorldDialogPrefab) as DevHubWorldDialog;
    _DevHubWorldDialogInstance.Initialize(_GamePrefabs);
    
    // Listen for dialog button tap events
    _DevHubWorldDialogInstance.OnDevButtonClicked += OnDevButtonClicked;
  }

  private void OnDevButtonClicked(GameBase miniGameClicked) {
    _DevHubWorldDialogInstance.OnDevButtonClicked -= OnDevButtonClicked;
    _DevHubWorldDialogInstance.CloseDialog();
    
    GameObject newMiniGameObject = GameObject.Instantiate(miniGameClicked.gameObject);
    _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
    _MiniGameInstance.OnMiniGameQuit += OnMiniGameQuit;
  }

  private void OnMiniGameQuit() {
    CloseMiniGame();
    ShowHubWorldDialog();
  }

  private void CloseMiniGame() {
    // Destroy game if it exists
    if (_MiniGameInstance != null) {
      _MiniGameInstance.CleanUp();
      Destroy(_MiniGameInstance.gameObject);
    }
  }
}

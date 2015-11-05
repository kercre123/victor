using UnityEngine;
using System.Collections;

public class DevHubWorld : HubWorldBase {

  [SerializeField]
  private GameBase[] gamePrefabs_;
 
  [SerializeField]
  private DevHubWorldDialog devHubWorldDialogPrefab_;
  private DevHubWorldDialog devHubWorldDialogInstance_;

  private GameBase miniGameInstance_;

  public override bool LoadHubWorld() {
    ShowHubWorldDialog();
    return true;
  }
  
  public override bool DestroyHubWorld() {
    
    // Deregister events
    // Destroy dialog if it exists
    if (devHubWorldDialogInstance_ != null) {
      devHubWorldDialogInstance_.OnDevButtonClicked -= OnDevButtonClicked;
      devHubWorldDialogInstance_.CloseDialogImmediately();
    }
    
    CloseMiniGame();
    return true;
  }

  private void ShowHubWorldDialog(){
    // Create dialog with the game prefabs
    devHubWorldDialogInstance_ = UIManager.OpenDialog(devHubWorldDialogPrefab_) as DevHubWorldDialog;
    devHubWorldDialogInstance_.Initialize(gamePrefabs_);
    
    // Listen for dialog button tap events
    devHubWorldDialogInstance_.OnDevButtonClicked += OnDevButtonClicked;
  }

  private void OnDevButtonClicked (GameBase miniGameClicked)
  {
    devHubWorldDialogInstance_.OnDevButtonClicked -= OnDevButtonClicked;
    devHubWorldDialogInstance_.CloseDialog();
    
    GameObject newMiniGameObject = GameObject.Instantiate(miniGameClicked.gameObject);
    miniGameInstance_ = newMiniGameObject.GetComponent<GameBase>();
    miniGameInstance_.OnMiniGameQuit += OnMiniGameQuit;
  }

  private void OnMiniGameQuit () {
    CloseMiniGame();
    ShowHubWorldDialog();
  }

  private void CloseMiniGame(){
    // Destroy game if it exists
    if (miniGameInstance_ != null) {
      miniGameInstance_.CleanUp();
      Destroy(miniGameInstance_.gameObject);
    }
  }
}

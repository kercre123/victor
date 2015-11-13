using UnityEngine;
using System.Collections;

public class HubWorld : HubWorldBase {

  [SerializeField]
  private HubWorldDialog _HubWorldDialogPrefab;
  private HubWorldDialog _HubWorldDialogInstance;

  private GameBase _MiniGameInstance;

  public override bool LoadHubWorld() {
    ShowHubWorldDialog();
    return true;
  }

  public override bool DestroyHubWorld() {
    // Deregister events
    // Destroy dialog if it exists
    if (_HubWorldDialogInstance != null) {
      _HubWorldDialogInstance.OnButtonClicked -= OnDevButtonClicked;
      _HubWorldDialogInstance.CloseDialogImmediately();
    }

    CloseMiniGame();
    return true;
  }

  private void ShowHubWorldDialog() {

    // Create dialog with the game prefabs
    _HubWorldDialogInstance = UIManager.OpenDialog(_HubWorldDialogPrefab) as HubWorldDialog;

  }

  private void OnDevButtonClicked(GameBase miniGameClicked) {
    _HubWorldDialogInstance.OnButtonClicked -= OnDevButtonClicked;
    _HubWorldDialogInstance.CloseDialog();

    GameObject newMiniGameObject = GameObject.Instantiate(miniGameClicked.gameObject);
    _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
    _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;
  }

  private void HandleMiniGameQuit() {
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

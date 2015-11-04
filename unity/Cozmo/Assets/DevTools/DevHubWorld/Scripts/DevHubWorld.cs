using UnityEngine;
using System.Collections;

public class DevHubWorld : HubWorldBase {

  [SerializeField]
  private GameBase[] gamePrefabs_;
 
  [SerializeField]
  private DevHubWorldDialog devHubWorldDialogPrefab_;

  private GameBase miniGameInstance_;

  public override bool LoadHubWorld() {
    //GameObject newMiniGameObject = GameObject.Instantiate(gamePrefab_.gameObject);
    //miniGameInstance_ = newMiniGameObject.GetComponent<GameBase>();

    // Create dialog with the game prefabs

    // Listen for dialog button tap events
    return true;
  }

  public override bool DestroyHubWorld() {
    // Destroy game if it exists
    if (miniGameInstance_ != null) {
      miniGameInstance_.CleanUp();
      Destroy(miniGameInstance_.gameObject);
    }
    return true;
  }
}

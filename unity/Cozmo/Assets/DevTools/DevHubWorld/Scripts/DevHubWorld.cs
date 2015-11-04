using UnityEngine;
using System.Collections;

public class DevHubWorld : HubWorldBase {

  [SerializeField]
  private GameBase gamePrefab_;
  private GameBase miniGameInstance_;

  public override bool LoadHubWorld() {
    GameObject newMiniGameObject = GameObject.Instantiate(gamePrefab_.gameObject);
    miniGameInstance_ = newMiniGameObject.GetComponent<GameBase>();
    return true;
  }

  public override bool DestroyHubWorld() {
    miniGameInstance_.HandleHubWorldDestroyed();
    Destroy(miniGameInstance_.gameObject);
    return true;
  }
}

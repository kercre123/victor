using UnityEngine;
using System.Collections;

public class UIPrefabHolder : ScriptableObject {

  private static UIPrefabHolder _Instance;

  public static UIPrefabHolder Instance {
    get {
      if (_Instance == null) {
        _Instance = Resources.Load<UIPrefabHolder>("Prefabs/UI/UIPrefabHolder");
      }
      return _Instance;
    }
    private set { }
  }

  [SerializeField]
  private GameObject _FullScreenButtonPrefab;

  public GameObject FullScreenButtonPrefab {
    get { return _FullScreenButtonPrefab; }
    set { }
  }
}

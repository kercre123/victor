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
  }

  [SerializeField]
  private GameObject _FullScreenButtonPrefab;

  public GameObject FullScreenButtonPrefab {
    get { return _FullScreenButtonPrefab; }
  }

  [SerializeField]
  private CozmoUiUtils.SimpleAlertView _AlertViewPrefab;

  public CozmoUiUtils.SimpleAlertView AlertViewPrefab {
    get { return _AlertViewPrefab; }
  }

  [SerializeField]
  private GameObject _DefaultQuitGameButtonPrefab;

  public GameObject DefaultQuitGameButtonPrefab {
    get { return _DefaultQuitGameButtonPrefab; }
  }
}

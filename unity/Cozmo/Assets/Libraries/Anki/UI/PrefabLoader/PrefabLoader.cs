using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PrefabLoader : MonoBehaviour {

  private const string _PrefabPath = "UIPrefabs";
  private static PrefabLoader _Instance;
  private static Anki.AppResources.PrefabCache _PrefabCache;

  public static PrefabLoader Instance {
    get { 
      if (null == _Instance) {
        PrefabLoader.Initialize();
      }
      return _Instance;
    }
  }

  public static void Initialize()
  {
    DAS.Debug("PrefabLoader.Initialize", "Create Instance");
    if (_Instance != null) {
      return;
    }
    _Instance = new GameObject("PrefabLoader").AddComponent<PrefabLoader>();
    _PrefabCache = new Anki.AppResources.PrefabCache();
  }

  public GameObject InstantiatePrefab(string name) {
    string prefabPath;
    GameObject prefab = null;
    if (_PrefabCache.TryGetPath(name, out prefabPath)) {
      DAS.Debug("PrefabLoader.InstantiatePrefab", "[PrefabCache] name: " + name + " path: " + prefabPath);
      prefab = Resources.Load<GameObject>(prefabPath);
    }

    if (prefab == null) {
      DAS.Debug("PrefabLoader.InstantiatePrefab", "[name as path] name: " + name);
      prefabPath = _PrefabPath + "/" + name;
      prefab = Resources.Load<GameObject>(prefabPath);
    }

    if (prefab == null) {
      DAS.Error("PrefabLoader.InstantiatePrefab",
        "Prefab not found: " + name + " Run ./configure.py and check PrefabCache.cs");
      return null;
    }

    DAS.Debug("PrefabLoader.InstantiatePrefab", "name: " + name);
    GameObject go = Instantiate(prefab)as GameObject;
    go.name = name;

    return go;
  }

  void OnDisable() {
    DAS.Debug("PrefabLoader.OnDisable", "Unloading Unused Prefabs");
    Resources.UnloadUnusedAssets();
  }

}

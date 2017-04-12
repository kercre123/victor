using UnityEngine;
using System.Collections;
using UnityEditor;

[CustomEditor(typeof(PrefabProxy))]
public class PrefabProxyEditor : Editor {

  private bool _Initialized = false;

  private GameObject _LastPrefab;

  public override void OnInspectorGUI() {
    base.OnInspectorGUI();

    PrefabProxy proxy = (PrefabProxy)target;
    if (!_Initialized) {
      _LastPrefab = proxy.Prefab;
      _Initialized = true;
    }
    else {
      if (_LastPrefab != proxy.Prefab) {
        for (int i = proxy.transform.childCount - 1; i >= 0; i--) {
          GameObject.DestroyImmediate(proxy.transform.GetChild(i).gameObject);
        }
        proxy.Expand();
        _LastPrefab = proxy.Prefab;
      }
    }
  }

}

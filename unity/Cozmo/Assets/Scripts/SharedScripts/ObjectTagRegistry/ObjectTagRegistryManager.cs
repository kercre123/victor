using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class ObjectTagRegistryManager : MonoBehaviour {

  private static readonly IDAS sDAS = DAS.GetInstance(typeof(ObjectTagRegistryManager));

  private static ObjectTagRegistryManager _Instance;

  public static ObjectTagRegistryManager Instance {
    get {
      if (_Instance == null) {
        sDAS.Error("Don't access this until Start!");
      }
      return _Instance;
    }
    private set {
      if (_Instance != null) {
        sDAS.Error("There shouldn't be more than one ObjectTagRegistryManager");
      }
      _Instance = value;
    }
  }

  private Dictionary<string, GameObject> _Registry = new Dictionary<string, GameObject>();

  void Awake() {
    Instance = this;
  }

  public void RegisterTag(string tag, GameObject objectRef) {
    if (_Registry.ContainsKey(tag)) {
      DAS.Error(this, "Object tag already exist!");
    }
    else {
      _Registry.Add(tag, objectRef);
    }
  }

  public void RemoveTag(string tag) {
    _Registry.Remove(tag);
  }

  public GameObject GetObjectByTag(string tag) {
    return _Registry[tag];
  }

}

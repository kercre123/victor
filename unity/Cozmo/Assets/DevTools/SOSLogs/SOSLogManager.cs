using UnityEngine;
using System.Collections;

public class SOSLogManager : MonoBehaviour {

  public static SOSLogManager Instance { get; private set; }

  private void OnEnable() {
    if (Instance != null && Instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
      DontDestroyOnLoad(gameObject);
    }
  }

  public void CreateListener() {
    
  }
}

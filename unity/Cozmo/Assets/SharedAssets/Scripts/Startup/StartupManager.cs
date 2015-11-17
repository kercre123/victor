using UnityEngine;
using System.Collections;

/// <summary>
/// Add managers to this object by calling
/// gameObject.AddComponent<ManagerTypeName>();
/// The managers should be simple enough to not require any 
/// serialized fields.
/// If the manager is not a MonoBehaviour, just create 
/// the object and cache it. 
/// </summary>
public class StartupManager : MonoBehaviour {

  // Use this for initialization
  void Start() {
    if (Application.isPlaying) {
      // Set up localization files.
      Localization.LoadStrings();

      // Add managers to this object here
      // gameObject.AddComponent<ManagerTypeName>();
    }
  }
}

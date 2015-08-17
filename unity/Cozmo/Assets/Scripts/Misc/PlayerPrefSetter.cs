using UnityEngine;
using System.Collections;

/// <summary>
/// simple component allowing the setting of a specific player pref
///   quite useful for causing ui interactions such as button presses to set a pref
/// </summary>
public class PlayerPrefSetter : MonoBehaviour {

  [SerializeField] string pref = null;

  public void SetPlayerPref(int val) {
    PlayerPrefs.SetInt(pref, val);
  }

  public void SetPlayerPref(string val) {
    PlayerPrefs.SetString(pref, val);
  }
}

using UnityEngine;
using System.Collections;

/// <summary>
/// slap this component on any objects that you only want to show up when the ShowDebugInfo pref is set from the options menu
///    this pref defaults to hiding all such objects
/// </summary>
public class DebugObject : MonoBehaviour
{

  void Awake()
  {
    RefreshSettings();
    OptionsScreen.RefreshSettings += RefreshSettings;
  }

  void OnDestroy()
  {
    OptionsScreen.RefreshSettings -= RefreshSettings;
  }

  void RefreshSettings()
  {
    gameObject.SetActive(PlayerPrefs.GetInt("ShowDebugInfo", 0) == 1);
  }

}

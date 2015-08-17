using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

[Serializable]
public class RectTransformSettings {
  public Vector2 anchoredPosition = new Vector2( 0f, 0f );
  public Vector3 anchoredPosition3D = new Vector3( 0f, 0f, 0f );
  public Vector2 anchorMax = new Vector2( 0.5f, 0.5f );
  public Vector2 anchorMin = new Vector2( 0.5f, 0.5f );
  public Vector2 offsetMax = new Vector2( 0f, 0f );
  public Vector2 offsetMin = new Vector2( 0f, 0f );
  public Vector2 pivot = new Vector2( 0.5f, 0.5f );
  public Vector2 sizeDelta = new Vector2( 100f, 100f );
  public Vector3 localScale = new Vector3( 1f, 1f, 1f );
}

/// <summary>
/// this component can be added to any gameobject with a RectTransform to allow it to switch between distinct settings for
///    distinct screen resolutions or aspect ratios.
///  relies on ScreenMultiSettingsDetector to detect screen changes and propagate setting swaps via subscription to a
///     static Action delegate.
/// </summary>
[ExecuteInEditMode]
public class RectTransformMultiSettings : MonoBehaviour {
  [SerializeField] bool debug = false;
  [SerializeField] List<RectTransformSettings> screenSettings = new List<RectTransformSettings>();

  bool smallScreen = false;
  RectTransform _rTrans;
  RectTransform rTrans {
    get {
      if(_rTrans == null) {
        _rTrans = transform as RectTransform;
        if(debug) Debug.Log(gameObject.name + " _rTrans = transform as RectTransform;");
      }
      return _rTrans;
    }
  }

  void Awake() {
    if(debug) Debug.Log(gameObject.name + " Awake");
  }

  void OnEnable() {
    if(debug) Debug.Log(gameObject.name + " OnEnable");
    RefreshSettingsToScreen();
    ScreenMultiSettingsDetector.ShareSettings += RefreshSettingsToScreen;
  }

  void OnDisable() {
    if(debug) Debug.Log(gameObject.name + " OnDisable");
    ScreenMultiSettingsDetector.ShareSettings -= RefreshSettingsToScreen;
  }

  void OnDestroy() {
    if(debug) Debug.Log(gameObject.name + " OnDestroy");
  }

  void RefreshSettingsToScreen() {
    
    int index = ScreenMultiSettingsDetector.CurrentIndex;
    while(screenSettings.Count <= index) {
      RectTransformSettings newSettings = new RectTransformSettings();
      SaveSettings(newSettings);
      screenSettings.Add(newSettings);
    }

    LoadSettings(screenSettings[index]);
  }

  void LoadSettings(RectTransformSettings settings) {
    if(!SettingsAreDifferent(settings)) return;
    rTrans.anchoredPosition = settings.anchoredPosition;
    rTrans.anchoredPosition3D = settings.anchoredPosition3D;
    rTrans.anchorMax = settings.anchorMax;
    rTrans.anchorMin = settings.anchorMin;
    rTrans.offsetMax = settings.offsetMax;
    rTrans.offsetMin = settings.offsetMin;
    rTrans.pivot = settings.pivot;
    rTrans.sizeDelta = settings.sizeDelta;
    rTrans.localScale = settings.localScale;
    
    if(debug) Debug.Log(gameObject.name + " LoadSettings for screenMode("+ScreenMultiSettingsDetector.CurrentIndex+")");
  }
  
  void SaveSettings(RectTransformSettings settings) {
    if(!SettingsAreDifferent(settings)) return;
    settings.anchoredPosition = rTrans.anchoredPosition;
    settings.anchoredPosition3D = rTrans.anchoredPosition3D;
    settings.anchorMax = rTrans.anchorMax;
    settings.anchorMin = rTrans.anchorMin;
    settings.offsetMax = rTrans.offsetMax;
    settings.offsetMin = rTrans.offsetMin;
    settings.pivot = rTrans.pivot;
    settings.sizeDelta = rTrans.sizeDelta;
    settings.localScale = rTrans.localScale;
    if(debug) Debug.Log(gameObject.name + " SaveSettings for screenMode("+ScreenMultiSettingsDetector.CurrentIndex+")");
  }

  bool SettingsAreDifferent(RectTransformSettings settings) {
    if(rTrans.anchoredPosition != settings.anchoredPosition)     return true;
    if(rTrans.anchoredPosition3D != settings.anchoredPosition3D)   return true;
    if(rTrans.anchorMax != settings.anchorMax)             return true;
    if(rTrans.anchorMin != settings.anchorMin)             return true;
    if(rTrans.offsetMax != settings.offsetMax)             return true;
    if(rTrans.offsetMin != settings.offsetMin)             return true;
    if(rTrans.pivot != settings.pivot)                 return true;
    if(rTrans.sizeDelta != settings.sizeDelta)             return true;
    if(rTrans.localScale != settings.localScale)           return true;

    return false;
  }

  //this is called from the custom inspector RectTransformMultiSettingsEditor whenever settings are changed
  public void SaveCurrentSettings() {
    if(debug) Debug.Log(gameObject.name + " SaveCurrentSettings for screenMode("+ScreenMultiSettingsDetector.CurrentIndex+")");

    int index = ScreenMultiSettingsDetector.CurrentIndex;
    while(screenSettings.Count <= index) {
      RectTransformSettings newSettings = new RectTransformSettings();
      SaveSettings(newSettings);
      screenSettings.Add(newSettings);
    }
    
    SaveSettings(screenSettings[index]);
  }

}
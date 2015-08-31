using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using UnityEditor;
using System.Collections;

[CustomEditor(typeof(RectTransformMultiSettings))]
public class RectTransformMultiSettingsEditor : Editor {
  public override void OnInspectorGUI() {

    RectTransformMultiSettings rectTransformBasedOnScreenSize = target as RectTransformMultiSettings;
    DrawDefaultInspector();

    if (GUILayout.Button("Save Current Settings")) {
      rectTransformBasedOnScreenSize.SaveCurrentSettings();
    }
  }
}
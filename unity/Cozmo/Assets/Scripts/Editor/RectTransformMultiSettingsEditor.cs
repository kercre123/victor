using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using UnityEditor;
using System.Collections;

[CustomEditor(typeof(RectTransformMultiSettings))]
public class RectTransformMultiSettingsEditor : Editor {
  public override void OnInspectorGUI() {

    RectTransformMultiSettings rectTransformBasedOnScreenSize = target as RectTransformMultiSettings;

    //EditorGUI.BeginChangeCheck();

    DrawDefaultInspector();

    if (GUILayout.Button("Save Current Settings")) {
      rectTransformBasedOnScreenSize.SaveCurrentSettings();
    }

//    if (EditorGUI.EndChangeCheck())
//    {
//      Debug.Log(rectTransformBasedOnScreenSize.gameObject.name + " OnInspectorGUI EndChangeCheck true!");
//    }
  }
}
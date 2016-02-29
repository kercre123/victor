using UnityEditor;
using UnityEngine;
using System.Collections.Generic;

[CustomPropertyDrawer(typeof(LocalizedString))]
public class LocalizedStringDrawer : PropertyDrawer {

  private Dictionary<string, TemporaryLocalizationData> _TemporaryLocalizationDictionary = new Dictionary<string, TemporaryLocalizationData>();

  private class TemporaryLocalizationData {
    public bool Expanded = false;
    public string LocalizedStringFile = null;
    public string LocalizedString = null;
  }

  public override float GetPropertyHeight(SerializedProperty property, GUIContent label) {

    TemporaryLocalizationData tempData;
    if (_TemporaryLocalizationDictionary.TryGetValue(property.propertyPath, out tempData)) {
      if (tempData.Expanded) {
        return base.GetPropertyHeight(property, label) * 9;
      }
    }
    return base.GetPropertyHeight(property, label);
  }

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    EditorGUI.BeginProperty(position, label, property);

    var keyProp = property.FindPropertyRelative("Key");

    string key = keyProp.stringValue;

    TemporaryLocalizationData tempData;
    if (!_TemporaryLocalizationDictionary.TryGetValue(property.propertyPath, out tempData)) {
      tempData = new TemporaryLocalizationData();
      EditorDrawingUtility.InitializeLocalizationString(key, out tempData.LocalizedStringFile, out tempData.LocalizedString);
      _TemporaryLocalizationDictionary[property.propertyPath] = tempData;
    }

    if (tempData.Expanded) {
      position.height /= 9f;
    }
    // Draw label
    tempData.Expanded = EditorGUI.Foldout(position, tempData.Expanded, label);


    if (tempData.Expanded) {
      position.y += position.height;
      position.height *= 8f;

      EditorDrawingUtility.DrawLocalizationString(position, ref key, ref tempData.LocalizedStringFile, ref tempData.LocalizedString);

      keyProp.stringValue = key;
    }
    else {
      position.width /= 2;
      position.x += position.width;
      // preview the current value
      GUI.Label(position, key);
    }
    EditorGUI.EndProperty ();
  }
}
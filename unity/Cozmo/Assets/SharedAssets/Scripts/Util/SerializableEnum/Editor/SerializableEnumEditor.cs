// from http://www.scottwebsterportfolio.com/blog/2015/3/29/enum-serialization-in-unity

using UnityEngine;
using System.Collections;
using UnityEditor;
using System;

[CustomPropertyDrawer(typeof(UnlockableInfo.SerializableUnlockIds))]
public class SerializableEnumEditor : PropertyDrawer {

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    EditorGUI.BeginProperty(position, label, property);

    position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID(FocusType.Passive), label);

    SerializedProperty enumProperty = property.FindPropertyRelative("_EnumValue");
    SerializedProperty enumStringProperty = property.FindPropertyRelative("_EnumValueAsString");

    for (int nameIndex = 0; nameIndex < enumProperty.enumNames.Length; nameIndex++) {
      if (enumProperty.enumNames[nameIndex] == enumStringProperty.stringValue) {
        enumProperty.enumValueIndex = nameIndex;
        break;
      }
    }

    enumProperty.enumValueIndex = EditorGUI.Popup(position, enumProperty.enumValueIndex, enumProperty.enumNames);
    enumStringProperty.stringValue = enumProperty.enumNames[enumProperty.enumValueIndex];

    EditorGUI.EndProperty();
  }
}

[CustomPropertyDrawer(typeof(SerializableGameEvents))]
public class SerializableGameEventEnumEditor : PropertyDrawer {

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    EditorGUI.BeginProperty(position, label, property);

    position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID(FocusType.Passive), label);

    SerializedProperty enumProperty = property.FindPropertyRelative("_EnumValue");
    SerializedProperty enumStringProperty = property.FindPropertyRelative("_EnumValueAsString");

    for (int nameIndex = 0; nameIndex < enumProperty.enumNames.Length; nameIndex++) {
      if (enumProperty.enumNames[nameIndex] == enumStringProperty.stringValue) {
        enumProperty.enumValueIndex = nameIndex;
        break;
      }
    }

    enumProperty.enumValueIndex = EditorGUI.Popup(position, enumProperty.enumValueIndex, enumProperty.enumNames);
    enumStringProperty.stringValue = enumProperty.enumNames[enumProperty.enumValueIndex];

    EditorGUI.EndProperty();
  }
}

[CustomPropertyDrawer(typeof(SerializableAnimationTrigger))]
public class SerializableAnimationTriggerEnumEditor : PropertyDrawer {

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    EditorGUI.BeginProperty(position, label, property);

    position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID(FocusType.Passive), label);
    SerializedProperty enumProperty = property.FindPropertyRelative("_EnumValue");
    SerializedProperty enumStringProperty = property.FindPropertyRelative("_EnumValueAsString");
    for (int nameIndex = 0; nameIndex < enumProperty.enumNames.Length; nameIndex++) {
      if (enumProperty.enumNames[nameIndex] == enumStringProperty.stringValue) {
        enumProperty.enumValueIndex = nameIndex;
        break;
      }
    }

    enumProperty.enumValueIndex = EditorGUI.Popup(position, enumProperty.enumValueIndex, enumProperty.enumNames);
    enumStringProperty.stringValue = enumProperty.enumNames[enumProperty.enumValueIndex];
    EditorGUI.EndProperty();
  }
}


// from http://www.scottwebsterportfolio.com/blog/2015/3/29/enum-serialization-in-unity

using UnityEngine;
using System.Collections.Generic;
using UnityEditor;

public class SerializableEnumDrawingHelper {
  public static void PopulateEnumStringToValueMap(SerializedProperty property, Dictionary<string, int> _EnumStringToValueMap) {
    SerializedProperty enumProperty = property.FindPropertyRelative("_EnumValue");
    for (int nameIndex = 0; nameIndex < enumProperty.enumNames.Length; nameIndex++) {
      _EnumStringToValueMap.Add(enumProperty.enumNames[nameIndex], nameIndex);
    }
  }

  public static void OnGUIHelper(Rect position, SerializedProperty property, GUIContent label, Dictionary<string, int> enumStringToValueMap, bool enumStringToValueMapPopulated) {
    if (!enumStringToValueMapPopulated) {
      PopulateEnumStringToValueMap(property, enumStringToValueMap);
    }

    EditorGUI.BeginProperty(position, label, property);

    position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID(FocusType.Passive), label);
    SerializedProperty enumProperty = property.FindPropertyRelative("_EnumValue");
    SerializedProperty enumStringProperty = property.FindPropertyRelative("_EnumValueAsString");

    if (enumStringToValueMap.ContainsKey(enumStringProperty.stringValue)) {
      enumProperty.enumValueIndex = enumStringToValueMap[enumStringProperty.stringValue];
    }
    else {
      enumProperty.enumValueIndex = 0;
    }

    enumProperty.enumValueIndex = EditorGUI.Popup(position, enumProperty.enumValueIndex, enumProperty.enumNames);
    enumStringProperty.stringValue = enumProperty.enumNames[enumProperty.enumValueIndex];
    EditorGUI.EndProperty();
  }
}

[CustomPropertyDrawer(typeof(UnlockableInfo.SerializableUnlockIds))]
public class SerializableEnumEditor : PropertyDrawer {

  private Dictionary<string, int> _EnumStringToValueMap = new Dictionary<string, int>();
  private bool _EnumStringToValueMapPopulated = false;

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    SerializableEnumDrawingHelper.OnGUIHelper(position, property, label, _EnumStringToValueMap, _EnumStringToValueMapPopulated);
    _EnumStringToValueMapPopulated = true;
  }
}

[CustomPropertyDrawer(typeof(SerializableGameEvents))]
public class SerializableGameEventEnumEditor : PropertyDrawer {

  private Dictionary<string, int> _EnumStringToValueMap = new Dictionary<string, int>();
  private bool _EnumStringToValueMapPopulated = false;

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    SerializableEnumDrawingHelper.OnGUIHelper(position, property, label, _EnumStringToValueMap, _EnumStringToValueMapPopulated);
    _EnumStringToValueMapPopulated = true;
  }
}

[CustomPropertyDrawer(typeof(SerializableAnimationTrigger))]
public class SerializableAnimationTriggerEnumEditor : PropertyDrawer {

  private Dictionary<string, int> _EnumStringToValueMap = new Dictionary<string, int>();
  private bool _EnumStringToValueMapPopulated = false;

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    SerializableEnumDrawingHelper.OnGUIHelper(position, property, label, _EnumStringToValueMap, _EnumStringToValueMapPopulated);
    _EnumStringToValueMapPopulated = true;
  }
}


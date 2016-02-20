using UnityEngine;
using UnityEditor;
using System.Collections;
using System;
using System.Linq;

[CustomPropertyDrawer(typeof(MusicStateWrapper))]
public class MusicStateWrapperDrawer : PropertyDrawer {

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    EditorGUI.BeginProperty(position, label, property);

    // Draw label
    position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID(FocusType.Passive), label);

    var type = typeof(Anki.Cozmo.Audio.GameState.Music);

    var options = Enum.GetNames(type);
    var values = Enum.GetValues(type).Cast<Enum>().Select(x => (int)(uint)(object)x).ToArray();

    var eventProp = property.FindPropertyRelative("_Music");
    eventProp.intValue = EditorGUI.IntPopup(position, eventProp.intValue, options, values);

    EditorGUI.EndProperty();
  }
}

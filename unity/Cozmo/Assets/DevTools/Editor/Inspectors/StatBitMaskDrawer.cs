using UnityEngine;
using System.Collections.Generic;
using UnityEditor;
using Anki.Cozmo;
using System.Linq;

[CustomPropertyDrawer(typeof(StatBitMask))]
public class StatBitMaskDrawer : PropertyDrawer {

  private static string[] _MaskOptions;

  static StatBitMaskDrawer() {
    _MaskOptions = System.Enum.GetNames(typeof(ProgressionStatType)).Where(x => x != "Count").ToArray();
  }

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    EditorGUI.BeginProperty(position, label, property);

    // Draw label
    position = EditorGUI.PrefixLabel (position, GUIUtility.GetControlID (FocusType.Passive), label);

    var maskProp = property.FindPropertyRelative("_Mask");

    maskProp.intValue = EditorGUI.MaskField(position, maskProp.intValue, _MaskOptions) % (1 << (int)ProgressionStatType.Count);

    EditorGUI.EndProperty ();
  }
}

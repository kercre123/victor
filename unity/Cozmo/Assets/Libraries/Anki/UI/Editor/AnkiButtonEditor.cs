using UnityEngine;
using UnityEditor;
using UnityEditor.UI;
using System.Collections;

namespace Anki.Editor.UI {
  [CustomEditor(typeof(Anki.UI.AnkiButton))]
  [CanEditMultipleObjects]
  public class AnkiButtonEditor : SelectableEditor {
    
    SerializedProperty _OnClickProperty;
    SerializedProperty _SoundEventProperty;
    SerializedProperty _DASEventProperty;
    
    protected override void OnEnable() {
      base.OnEnable();
      _OnClickProperty = serializedObject.FindProperty("_OnClick");
      _SoundEventProperty = serializedObject.FindProperty("_UISoundEvent");
      _DASEventProperty = serializedObject.FindProperty("_DASEventName");
    }
    
    public override void OnInspectorGUI() {
      base.OnInspectorGUI();
      EditorGUILayout.Space();
      serializedObject.Update();
      EditorGUILayout.PropertyField(_SoundEventProperty);
      EditorGUILayout.PropertyField(_DASEventProperty);
      EditorGUILayout.Space();
      EditorGUILayout.PropertyField(_OnClickProperty);
      serializedObject.ApplyModifiedProperties();
      if (GUI.changed) {
        EditorUtility.SetDirty(target);
      }
    }
    
  }
}

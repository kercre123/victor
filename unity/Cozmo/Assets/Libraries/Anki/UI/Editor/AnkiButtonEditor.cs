using UnityEngine;
using UnityEditor;
using UnityEditor.UI;
using System.Collections;

namespace Anki.Editor.UI {
  [CustomEditor(typeof(Anki.UI.AnkiButton))]
  [CanEditMultipleObjects]
  public class AnkiButtonEditor : SelectableEditor {
    
    SerializedProperty _OnClickProperty;
    //SerializedProperty _SoundEventProperty;
    SerializedProperty _DASEventProperty;
    SerializedProperty _TextFieldProperty;
    SerializedProperty _CanvasGroupProperty;

    protected override void OnEnable() {
      base.OnEnable();
      _OnClickProperty = serializedObject.FindProperty("_OnClick");
      //_SoundEventProperty = serializedObject.FindProperty("_UISoundEvent");
      _DASEventProperty = serializedObject.FindProperty("_DASEventName");
      _TextFieldProperty = serializedObject.FindProperty("_TextLabel");
      _CanvasGroupProperty = serializedObject.FindProperty("_AlphaController");
    }

    public override void OnInspectorGUI() {
      base.OnInspectorGUI();
      EditorGUILayout.Space();
      serializedObject.Update();
      //EditorGUILayout.PropertyField(_SoundEventProperty);
      EditorGUILayout.PropertyField(_DASEventProperty);
      EditorGUILayout.PropertyField(_TextFieldProperty);
      EditorGUILayout.PropertyField(_CanvasGroupProperty);
      EditorGUILayout.Space();
      EditorGUILayout.PropertyField(_OnClickProperty);
      serializedObject.ApplyModifiedProperties();
      if (GUI.changed) {
        EditorUtility.SetDirty(target);
      }
    }
    
  }
}

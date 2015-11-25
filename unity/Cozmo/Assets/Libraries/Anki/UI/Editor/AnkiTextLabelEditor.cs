using UnityEngine;
using UnityEngine.UI;
using UnityEditor;
using UnityEditor.UI;
using System.Collections;

namespace Anki.Editor.UI {

  [CustomEditor(typeof(Anki.UI.AnkiTextLabel), true)]
  [CanEditMultipleObjects]
  public class AnkiTextEditor : GraphicEditor {
    SerializedProperty m_Text;
    SerializedProperty m_FontData;

    protected override void OnEnable() {
      base.OnEnable();
      m_Text = serializedObject.FindProperty("m_Text");
      m_FontData = serializedObject.FindProperty("m_FontData");
    }

    public override void OnInspectorGUI() {
      serializedObject.Update();

      EditorGUILayout.PropertyField(m_Text);
      EditorGUILayout.PropertyField(m_FontData);
      AppearanceControlsGUI();
      serializedObject.ApplyModifiedProperties();
      if (GUI.changed) {
        EditorUtility.SetDirty(target);
      }
    }
  }

}

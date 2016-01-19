using UnityEngine;
using UnityEngine.UI;
using UnityEditor;
using UnityEditor.UI;
using System.Collections;
using System.Linq;

namespace Anki.Editor.UI {

  [CustomEditor(typeof(Anki.UI.AnkiTextLabel), true)]
  [CanEditMultipleObjects]
  public class AnkiTextEditor : GraphicEditor {
    SerializedProperty m_Text;
    SerializedProperty m_FontData;
    string _LocalizedString;
    string _LocalizedStringFile;
    string _LocalizationKey;
    bool _Localized;

    protected override void OnEnable() {
      base.OnEnable();
      m_Text = serializedObject.FindProperty("m_Text");
      m_FontData = serializedObject.FindProperty("m_FontData");

      var currentValue = m_Text.stringValue;
      if (currentValue.StartsWith("#")) {
        _Localized = true;
        _LocalizationKey = currentValue.Substring(1);
        _LocalizedString = LocalizationEditorUtility.GetTranslation(_LocalizationKey, out _LocalizedStringFile);
      }
      else {
        _LocalizedString = currentValue;
        _LocalizationKey = string.Empty;
        _LocalizedStringFile = LocalizationEditorUtility.LocalizationFiles.FirstOrDefault();
      }
    }

    public override void OnInspectorGUI() {
      serializedObject.Update();

      _Localized = EditorGUILayout.Toggle("Localize", _Localized);

      m_Text.stringValue = _LocalizedString;
      if (_Localized) {
        int selectedFileIndex = EditorGUILayout.Popup("Localization File", 
          Mathf.Max(0, 
                    System.Array.IndexOf(
                      LocalizationEditorUtility.LocalizationFiles, 
                      _LocalizedStringFile)), 
          LocalizationEditorUtility.LocalizationFiles);
        _LocalizedStringFile = LocalizationEditorUtility.LocalizationFiles[selectedFileIndex];

        _LocalizationKey = EditorGUILayout.TextField("Localization Key", _LocalizationKey);

        if (LocalizationEditorUtility.GetTranslation(_LocalizedStringFile, _LocalizationKey) != _LocalizedString) {
          EditorGUILayout.BeginHorizontal();
          if (GUILayout.Button("Reset")) {
            _LocalizedString = LocalizationEditorUtility.GetTranslation(_LocalizedStringFile, _LocalizationKey);
            m_Text.stringValue = _LocalizedString;
          }
          if (GUILayout.Button("Save")) {
            LocalizationEditorUtility.SetTranslation(_LocalizedStringFile, _LocalizationKey, _LocalizedString);
          }
          EditorGUILayout.EndHorizontal();
        }

        EditorGUILayout.PropertyField(m_Text);
        _LocalizedString = m_Text.stringValue;

        m_Text.stringValue = "#" + _LocalizationKey;
      }
      else {        
        EditorGUILayout.PropertyField(m_Text);
        _LocalizedString = m_Text.stringValue;
      }

      EditorGUILayout.PropertyField(m_FontData);
      AppearanceControlsGUI();
      serializedObject.ApplyModifiedProperties();
      if (GUI.changed) {
        EditorUtility.SetDirty(target);
      }
    }
  }

}

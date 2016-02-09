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
    SerializedProperty _AllUppercase;
    SerializedProperty _GlowText;

    Material _TextGlowMat;

    protected override void OnEnable() {
      base.OnEnable();

      _TextGlowMat = Resources.Load<Material>("Fonts/TextGlow");

      m_Text = serializedObject.FindProperty("m_Text");
      m_FontData = serializedObject.FindProperty("m_FontData");
      _AllUppercase = serializedObject.FindProperty("_AllUppercase");
      _GlowText = serializedObject.FindProperty("GlowText");

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

      if (_Localized) {
        EditorDrawingUtility.DrawLocalizationString(ref _LocalizationKey, ref _LocalizedStringFile, ref _LocalizedString);
        m_Text.stringValue = "#" + _LocalizationKey;
      }
      else {        
        m_Text.stringValue = _LocalizedString;
        EditorGUILayout.PropertyField(m_Text);
        _LocalizedString = m_Text.stringValue;
      }

      EditorGUILayout.PropertyField(_AllUppercase, new GUIContent("All Caps"));
      EditorGUILayout.PropertyField(_GlowText, new GUIContent("Glowing Text"));
      if (_GlowText.boolValue) {
        m_Material.objectReferenceValue = _TextGlowMat;
      }
      else {
        var oldRef = m_Material.objectReferenceValue;

        if (oldRef == _TextGlowMat) {
          m_Material.objectReferenceValue = null;
        }
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

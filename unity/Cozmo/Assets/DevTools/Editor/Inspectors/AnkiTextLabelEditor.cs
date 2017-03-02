using UnityEngine;
using UnityEngine.UI;
using UnityEditor;
using UnityEditor.UI;
using System.Collections;
using System.Linq;

namespace Anki.Editor.UI {

  [CustomEditor(typeof(Anki.UI.AnkiTextLegacy), true)]
  [CanEditMultipleObjects]
  public class AnkiTextEditor : GraphicEditor {
    private SerializedProperty _Text;
    private SerializedProperty _FontData;
    private string _LocalizedString;
    private string _LocalizedStringFile;
    private string _LocalizationKey;
    private bool _Localized;
    private SerializedProperty _AllUppercase;
    private SerializedProperty _GlowText;

    private Material _TextGlowMat;
    private SerializedProperty _RaycastBlocker;

    protected override void OnEnable() {
      base.OnEnable();

      _TextGlowMat = Resources.Load<Material>("Fonts/TextGlow");

      _Text = serializedObject.FindProperty("m_Text");
      _FontData = serializedObject.FindProperty("m_FontData");
      _AllUppercase = serializedObject.FindProperty("_AllUppercase");
      _GlowText = serializedObject.FindProperty("GlowText");
      _RaycastBlocker = serializedObject.FindProperty("IsRaycastBlocker");

      var currentValue = _Text.stringValue;
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
        _Text.stringValue = "#" + _LocalizationKey;
      }
      else {
        _Text.stringValue = _LocalizedString;
        EditorGUILayout.PropertyField(_Text);
        _LocalizedString = _Text.stringValue;
      }

      EditorGUILayout.PropertyField(_AllUppercase, new GUIContent("All Caps"));
      EditorGUILayout.PropertyField(_GlowText, new GUIContent("Glowing Text"));
      EditorGUILayout.PropertyField(_RaycastBlocker, new GUIContent("Raycast Blocker"));
      if (_GlowText.boolValue) {
        m_Material.objectReferenceValue = _TextGlowMat;
      }
      else {
        var oldRef = m_Material.objectReferenceValue;

        if (oldRef == _TextGlowMat) {
          m_Material.objectReferenceValue = null;
        }
      }

      EditorGUILayout.PropertyField(_FontData);
      AppearanceControlsGUI();
      serializedObject.ApplyModifiedProperties();
      if (GUI.changed) {
        EditorUtility.SetDirty(target);
      }
    }
  }

}

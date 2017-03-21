using Anki.Core.Editor.Components;
using UnityEngine;
using UnityEngine.UI;
using UnityEditor;
using UnityEditor.UI;
using System.Collections;
using System.Linq;
using Anki.Core.UI.Components;
using UnityEditor.SceneManagement;

namespace Cozmo.UI {
  [CustomEditor(typeof(CozmoText))]
  [CanEditMultipleObjects]
  public class CozmoTextEditor : AnkiTextMeshProEditor {
    private SerializedProperty _Text;
    private SerializedProperty _TextAlignmentProperty;
    private string _LocalizedString;
    private string _LocalizedStringFile;
    private string _LocalizationKey;
    private bool _Localized;

    protected new void OnEnable() {
      base.OnEnable();

      _Text = serializedObject.FindProperty("m_text");
      _TextAlignmentProperty = serializedObject.FindProperty("m_textAlignment");

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
      if (_ScriptTarget == null) {
        _ScriptTarget = (CozmoText)target;
      }

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

      EditorGUILayout.PropertyField(serializedObject.FindProperty("m_fontSize"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("m_fontAsset"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("m_fontColor"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("m_fontStyle"));

      #region TextAlignment
      EditorGUI.BeginChangeCheck();

      Rect rect = EditorGUILayout.GetControlRect(false, 20);
      GUIStyle btn = new GUIStyle(GUI.skin.button);
      btn.margin = new RectOffset(1, 1, 1, 1);
      btn.padding = new RectOffset(1, 1, 1, 0);

      selAlignGrid_A = TMPro.EditorUtilities.TMP_EditorUtility.GetHorizontalAlignmentGridValue(_TextAlignmentProperty.intValue);
      selAlignGrid_B = TMPro.EditorUtilities.TMP_EditorUtility.GetVerticalAlignmentGridValue(_TextAlignmentProperty.intValue);

      GUI.Label(new Rect(rect.x, rect.y + 2, 100, rect.height), "Alignment");
      float columnB = EditorGUIUtility.labelWidth + 15;
      selAlignGrid_A = GUI.SelectionGrid(new Rect(columnB, rect.y, 23 * 6, rect.height), selAlignGrid_A, TMPro.EditorUtilities.TMP_UIStyleManager.alignContent_A, 6, btn);
      selAlignGrid_B = GUI.SelectionGrid(new Rect(columnB, rect.y + 25, 23 * 6, rect.height), selAlignGrid_B, TMPro.EditorUtilities.TMP_UIStyleManager.alignContent_B, 6, btn);

      if (EditorGUI.EndChangeCheck()) {
        int value = (0x1 << selAlignGrid_A) | (0x100 << selAlignGrid_B);
        _TextAlignmentProperty.intValue = value;
      }

      EditorGUILayout.Space();
      EditorGUILayout.Space();
      EditorGUILayout.Space();
      EditorGUILayout.Space();

      #endregion

      EditorGUILayout.PropertyField(serializedObject.FindProperty("m_overflowMode"));

      _ScriptTarget.enableAutoSizing = EditorGUILayout.Toggle("Auto Sizing", _ScriptTarget.enableAutoSizing);
      if (_ScriptTarget.enableAutoSizing) {
        _ScriptTarget.fontSizeMin = EditorGUILayout.FloatField("Auto Size Min", _ScriptTarget.fontSizeMin);
        _ScriptTarget.fontSizeMax = EditorGUILayout.FloatField("Auto Size Max", _ScriptTarget.fontSizeMax);
      }

      DrawTMPDefaultSkinningOptions();

      serializedObject.ApplyModifiedProperties();
      if (GUI.changed) {
        EditorUtility.SetDirty(target);
      }
    }

    public override void DrawRuntimeSkinningOptions() {
      base.DrawRuntimeSkinningOptions();
    }

    public override void DrawSavingOptionToggles() {
      base.DrawSavingOptionToggles();
    }

    public override void UpdateSkinnableElementsForSaving(ref ThemesJson.ThemeComponentObj themeComponentObj) {
      base.UpdateSkinnableElementsForSaving(ref themeComponentObj);
    }

    private void DrawTMPDefaultSkinningOptions() {

      //Get Target Script
      if (_ScriptTarget == null) {
        _ScriptTarget = (CozmoText)target;
      }

      ThemeSystemEditorUtilsTMP.sInstance.DrawSkinningOptionsHeader();

      ThemeSystemEditorUtilsTMP.sInstance.DrawLinkedComponentIdDropdown(_ScriptTarget, ThemesJson.ComponentTypes.TextMeshPro);

      if (string.IsNullOrEmpty(_ScriptTarget.LinkedComponentId) && _ScriptTarget.IsPreview) {
        ThemeSystemEditorUtilsTMP.sInstance.ResetPreview(_ScriptTarget);
      }

      if (!string.IsNullOrEmpty(_ScriptTarget.LinkedComponentId)) {
        //Force load the json that is on disk
        ThemesJson.LoadJson();

        DrawRuntimeSkinningOptions();

        DrawEditorSkinningOptions();

        DrawSavingSkinningOptions();

        if (_ScriptTarget.IsPreview) {
          ThemeSystemEditorUtilsTMP.sInstance.DrawPreviewHelpBoxWarning(_ScriptTarget);
        }
      }
      else {
        DrawOnlyNewSavingSkinningOptions();
      }

      if (!Application.isPlaying) {
        //If we have changed anything lets make sure we mark our target as dirty
        if (GUI.changed) {
          EditorUtility.SetDirty(target);
          EditorSceneManager.MarkSceneDirty(EditorSceneManager.GetActiveScene());
        }
      }
      else {
        EditorGUILayout.HelpBox("Theme System changes while playing will not be saved!", MessageType.Warning);
      }

      GUILayout.Box("End of Theme System Options");
    }
  }

}

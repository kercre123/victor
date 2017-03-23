using TMPro.EditorUtilities;
using UnityEditor;
using Anki.Core.UI.Components;
using UnityEngine;
using UnityEditor.SceneManagement;

namespace Anki.Core.Editor.Components {
  [CustomEditor(typeof(AnkiTextMeshPro))]
  public class AnkiTextMeshProEditor : TMP_UiEditorPanel, ISkinnableComponentEditor {
    protected bool _SavingSkinFaceColor = true;
    protected bool _SavingSkinOutlineColor = true;
    protected bool _SavingSkinOutlineThickness = true;
    protected bool _SavingSkinFontSize = true;
    protected bool _SavingSkinFontStyle = true;
    protected bool _SavingTextAlignment = true;
    protected bool _SavingTextOverflow = true;
    protected bool _SavingAutoSizing = true;

    protected int _SavingSkinFontSizeOffset = 0;
    protected bool _SavingSkinFont = true;
    protected AnkiTextMeshPro _ScriptTarget = null;

    public override void OnInspectorGUI() {
      //Cache off default values
      float labelWidth = EditorGUIUtility.labelWidth;
      float fieldWidth = EditorGUIUtility.fieldWidth;
      int indentLevel = EditorGUI.indentLevel;

      base.OnInspectorGUI();

      //Reset our ui to use default values
      EditorGUIUtility.labelWidth = labelWidth;
      EditorGUIUtility.fieldWidth = fieldWidth;
      EditorGUI.indentLevel = indentLevel;

      //Get Target Script
      if (_ScriptTarget == null) {
        _ScriptTarget = (AnkiTextMeshPro)target;
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

    public virtual void DrawRuntimeSkinningOptions() {
      ThemeSystemEditorUtilsTMP.sInstance.DrawShowSkinningOptionsFoldout(ThemeSystemEditorUtilsTMP.SkinningFoldoutTypes.Runtime, _ScriptTarget);
      if (_ScriptTarget.ShowRuntimeSkinningOptions) {
        _ScriptTarget.OverrideSkinFaceTint = EditorGUILayout.Toggle("Use Prefab Face Tint", _ScriptTarget.OverrideSkinFaceTint);
        _ScriptTarget.OverrideOutlineColor = EditorGUILayout.Toggle("Use Prefab Outline Tint", _ScriptTarget.OverrideOutlineColor);
        _ScriptTarget.OverrideOutlineThickness = EditorGUILayout.Toggle("Use Prefab Outline Thickness", _ScriptTarget.OverrideOutlineThickness);
        _ScriptTarget.OverrideSkinFontSize = EditorGUILayout.Toggle("Use Prefab Font Size", _ScriptTarget.OverrideSkinFontSize);
        _ScriptTarget.OverrideSkinFontStyle = EditorGUILayout.Toggle("Use Prefab Font Style", _ScriptTarget.OverrideSkinFontStyle);
        _ScriptTarget.OverrideSkinTextAlignment = EditorGUILayout.Toggle("Use Prefab Text Alignment", _ScriptTarget.OverrideSkinTextAlignment);
        _ScriptTarget.OverrideSkinTextOverflow = EditorGUILayout.Toggle("Use Prefab Text Overflow", _ScriptTarget.OverrideSkinTextOverflow);

        EditorGUILayout.Space();
      }
    }

    public virtual void DrawEditorSkinningOptions() {
      ThemeSystemEditorUtilsTMP.sInstance.DrawShowSkinningOptionsFoldout(ThemeSystemEditorUtilsTMP.SkinningFoldoutTypes.Editor, _ScriptTarget);
      if (_ScriptTarget.ShowEditorSkinningOptions) {
        ThemeSystemEditorUtilsTMP.sInstance.DrawSkinningPreviewOptionsDropdown();
        if (GUILayout.Button("Preview")) {
          ThemeSystemEditorUtilsTMP.sInstance.Preview(_ScriptTarget);
        }

        if (GUILayout.Button("Reset")) {
          ThemeSystemEditorUtilsTMP.sInstance.ResetPreview(_ScriptTarget);
        }

        EditorGUILayout.Space();
      }
    }

    public virtual void DrawSavingSkinningOptions() {
      ThemeSystemEditorUtilsTMP.sInstance.DrawShowSkinningOptionsFoldout(ThemeSystemEditorUtilsTMP.SkinningFoldoutTypes.Saving, _ScriptTarget);
      if (_ScriptTarget.ShowSavingSkinningOptions) {
        DrawSavingOptionToggles();
        ThemeSystemEditorUtilsTMP.sInstance.DrawSkinningSavingOptionsDropdown();
        DrawOverwriteOptions();
        DrawSaveNewOptions();
      }
    }

    public virtual void DrawOnlyNewSavingSkinningOptions() {
      ThemeSystemEditorUtilsTMP.sInstance.DrawShowSkinningOptionsFoldout(ThemeSystemEditorUtilsTMP.SkinningFoldoutTypes.Saving, _ScriptTarget);
      if (_ScriptTarget.ShowSavingSkinningOptions) {
        DrawSavingOptionToggles();
        DrawSaveNewOptions();
      }
    }

    public virtual void DrawSavingOptionToggles() {
      _SavingSkinFaceColor = EditorGUILayout.Toggle("Save Face Tint To Skin", _SavingSkinFaceColor);
      _SavingSkinOutlineColor = EditorGUILayout.Toggle("Save Outline Tint To Skin", _SavingSkinOutlineColor);
      _SavingSkinOutlineThickness = EditorGUILayout.Toggle("Save Outline Thickness To Skin", _SavingSkinOutlineThickness);
      _SavingSkinFontSize = EditorGUILayout.Toggle("Save Font Size To Skin", _SavingSkinFontSize);
      _SavingSkinFontSizeOffset = EditorGUILayout.IntField("Font Offset", _SavingSkinFontSizeOffset);
    }

    public virtual void DrawOverwriteOptions() {
      EditorGUILayout.BeginHorizontal();
      if (GUILayout.Button("Save Over Current ->>>", GUILayout.Width(200))) {
        if (ThemeSystemEditorUtilsTMP.sInstance.DisplayOverwriteSaveDialogue(_ScriptTarget)) {
          DoOverwrite();
        }
      }
      GUILayout.Box(_ScriptTarget.LinkedComponentId);
      EditorGUILayout.EndHorizontal();
    }

    public virtual void DrawSaveNewOptions() {
      EditorGUILayout.BeginHorizontal();
      if (GUILayout.Button("Create New ->>>", GUILayout.Width(200))) {
        if (!string.IsNullOrEmpty(ThemeSystemEditorUtilsTMP.NewSaveId) && !ThemeSystemEditorUtilsTMP.sInstance.IdAlreadyExist(
              ThemeSystemEditorUtilsTMP.sInstance.GetCategoryFromComponentType(ThemesJson.ComponentTypes.TextMeshPro), ThemeSystemEditorUtilsTMP.NewSaveId)) {
          if (ThemeSystemEditorUtilsTMP.sInstance.DisplayNewSaveDialogue()) {
            DoCreateNew();
          }
        }
        else {
          EditorUtility.DisplayDialog("Theme System", "Error creating new Component." + "" +
          "\nInput name cannot be empty or the same as a current component.", "OK");
        }
      }
      ThemeSystemEditorUtilsTMP.NewSaveId = EditorGUILayout.TextField(ThemeSystemEditorUtilsTMP.NewSaveId);
      EditorGUILayout.EndHorizontal();
    }

    public virtual void DoCreateNew() {
      //Create ThemeComponentObj and send it to themesjson to be saved 
      ThemesJson.ThemeComponentObj themeComponentObj = new ThemesJson.ThemeComponentObj();
      themeComponentObj.Id = ThemeSystemEditorUtilsTMP.NewSaveId;
      themeComponentObj.Type = ThemesJson.ComponentTypes.TextMeshPro;
      UpdateSkinnableElementsForSaving(ref themeComponentObj);

      //Add our themeComponentObj to the all themes/skins
      for (int i = 0; i < ThemesJson.CurrentlyLoadedInstance.Themes.Count; i++) {
        for (int j = 0; j < ThemesJson.CurrentlyLoadedInstance.Themes[i].Skins.Count; j++) {
          //Check to make sure there isnt already a component with this id, if there is we dont want to save another copy, instead we just update it
          int foundIndex = ThemesJson.CurrentlyLoadedInstance.Themes[i].Skins[j].Components.FindIndex(x => x.Id == themeComponentObj.Id);
          if (foundIndex == -1) {
            ThemesJson.CurrentlyLoadedInstance.Themes[i].Skins[j].Components.Add(themeComponentObj);
          }
          else {
            ThemesJson.CurrentlyLoadedInstance.Themes[i].Skins[j].Components[foundIndex] = themeComponentObj;
          }
        }
      }

      //Save the JSON
      ThemeSystemEditorUtilsTMP.sInstance.SaveJsonFromEditor(ThemesJson.CurrentlyLoadedInstance, ThemeSystemConfigJson.CurrentlyLoadedInstance);

      //Clear our save id
      ThemeSystemEditorUtilsTMP.NewSaveId = string.Empty;
    }

    public virtual void DoOverwrite() {
      //Create ThemeComponentObj and send it to themesjson to be saved 
      ThemesJson.ThemeComponentObj themeComponentObj = ThemeSystemEditorUtilsTMP.sInstance.GetThemeComponentForSaving(_ScriptTarget);

      UpdateSkinnableElementsForSaving(ref themeComponentObj);

      ThemeSystemEditorUtilsTMP.sInstance.SaveJsonFromEditor(ThemesJson.CurrentlyLoadedInstance, ThemeSystemConfigJson.CurrentlyLoadedInstance);
    }

    public virtual void UpdateSkinnableElementsForSaving(ref ThemesJson.ThemeComponentObj themeComponentObj) {
      themeComponentObj.SkinFontColor = _SavingSkinFaceColor;
      if (_SavingSkinFaceColor) {
        themeComponentObj.FontColor = _ScriptTarget.faceColor;
      }

      themeComponentObj.SkinTMPOutlineColor = _SavingSkinOutlineColor;
      if (_SavingSkinOutlineColor) {
        themeComponentObj.TMPOutlineColor = _ScriptTarget.outlineColor;
      }

      themeComponentObj.SkinTMPOutlineThickness = _SavingSkinOutlineThickness;
      if (_SavingSkinOutlineThickness) {
        themeComponentObj.TMPOutlineThickness = _ScriptTarget.outlineWidth;
      }

      themeComponentObj.SkinFontSize = _SavingSkinFontSize;
      if (_SavingSkinFontSize) {
        themeComponentObj.FontSizeOffset = _SavingSkinFontSizeOffset;
      }

      themeComponentObj.SkinFontStyle = _SavingSkinFontStyle;
      if (_SavingSkinFontStyle) {
        themeComponentObj.FontStyle = (int)_ScriptTarget.fontStyle;
      }

      themeComponentObj.SkinTextAlignment = _SavingTextAlignment;
      if (_SavingTextAlignment) {
        themeComponentObj.TextAlignment = (int)_ScriptTarget.alignment;
      }

      themeComponentObj.SkinTextOverflow = _SavingTextOverflow;
      if (_SavingTextOverflow) {
        themeComponentObj.TextOverflow = (int)_ScriptTarget.overflowMode;
      }

      themeComponentObj.SkinAutoSizing = _SavingAutoSizing;
      if (_SavingAutoSizing) {
        themeComponentObj.EnableAutoSizing = _ScriptTarget.enableAutoSizing;
        themeComponentObj.AutoSizingMin = _ScriptTarget.fontSizeMin;
        themeComponentObj.AutoSizingMax = _ScriptTarget.fontSizeMax;
      }

      themeComponentObj.SkinFont = _SavingSkinFont;
      if (_SavingSkinFont) {
        themeComponentObj.FontResourceKey = _ScriptTarget.font != null ? ThemeSystemEditorUtilsTMP.sInstance.GetAssetResourcesPath(_ScriptTarget.font) : string.Empty;
      }
    }
  }
}
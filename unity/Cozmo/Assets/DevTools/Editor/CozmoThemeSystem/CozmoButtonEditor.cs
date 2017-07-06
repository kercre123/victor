using Anki.Core.Editor.Components;
using Anki.Core.UI.Components;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace Cozmo.UI {
  [CustomEditor(typeof(CozmoButton), true)]
  [CanEditMultipleObjects]
  public class CozmoButtonEditor : AnkiButtonEditor {
    public override void OnInspectorGUI() {
      serializedObject.UpdateIfRequiredOrScript();

      //Get Target Script
      if (_ScriptTarget == null) {
        _ScriptTarget = (AnkiButton)target;
      }

      //Show the button here so it's at the top of the inspector.
      //Delay the action till after the other properties are applied.
      bool createDefaultComponents = false;
      if (_ScriptTarget.transform.childCount == 0) {
        createDefaultComponents = GUILayout.Button("Create Default Button");
      }

      EditorGUILayout.PropertyField(serializedObject.FindProperty("TextEnabledColor"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("TextPressedColor"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("TextDisabledColor"));

      EditorGUILayout.PropertyField(serializedObject.FindProperty("_TextLabel"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("_AlphaController"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("_GlintAnimator"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("_UISoundEvent"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("_ShowDisabledStateWhenInteractable"));
      EditorGUILayout.PropertyField(serializedObject.FindProperty("ButtonGraphics"), true);
      serializedObject.ApplyModifiedProperties();



      if (createDefaultComponents) {
        CreateDefaultComponents();
      }

      ThemeSystemEditorUtils.sInstance.DrawSkinningOptionsHeader();

      ThemeSystemEditorUtils.sInstance.DrawLinkedComponentIdDropdown(_ScriptTarget, ThemesJson.ComponentTypes.Button);

      if (string.IsNullOrEmpty(_ScriptTarget.LinkedComponentId) && _ScriptTarget.IsPreview) {
        ThemeSystemEditorUtils.sInstance.ResetPreview(_ScriptTarget);
      }

      if (!string.IsNullOrEmpty(_ScriptTarget.LinkedComponentId)) {
        //Force load the json that is on disk
        ThemesJson.LoadJson();

        DrawRuntimeSkinningOptions();

        DrawEditorSkinningOptions();

        DrawSavingSkinningOptions();

        if (_ScriptTarget.IsPreview) {
          ThemeSystemEditorUtils.sInstance.DrawPreviewHelpBoxWarning(_ScriptTarget);
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

    public override void DrawRuntimeSkinningOptions() {
      CozmoButton CozmoButtonScriptTarget = (CozmoButton)_ScriptTarget;
      ThemeSystemEditorUtils.sInstance.DrawShowSkinningOptionsFoldout(ThemeSystemEditorUtils.SkinningFoldoutTypes.Runtime, _ScriptTarget);

      if (_ScriptTarget.ShowRuntimeSkinningOptions) {
        _ScriptTarget.OverrideSkinTransition = EditorGUILayout.Toggle("Use Prefab Transition", _ScriptTarget.OverrideSkinTransition);
        CozmoButtonScriptTarget.OverrideButtonImageArray = EditorGUILayout.Toggle("Use Prefab ButtonImage Array", CozmoButtonScriptTarget.OverrideButtonImageArray);
      }

      EditorGUILayout.Space();

    }

    //Creates default children and components for Button Image, Glint, and Text Label
    private void CreateDefaultComponents() {
      if (_ScriptTarget == null) {
        _ScriptTarget = (AnkiButton)target;
      }
      CozmoButton button = _ScriptTarget as CozmoButton;
      CozmoButton.AnkiButtonImage[] buttonGraphics = new CozmoButton.AnkiButtonImage[2];
      GameObject go;
      RectTransform rt;
      go = new GameObject("ButtonImage", typeof(CozmoImage));
      CozmoImage buttonImage = go.GetComponent<CozmoImage>();
      go.transform.SetParent(button.transform, false);
      buttonGraphics[0] = new CozmoButton.AnkiButtonImage() {
        targetImage = buttonImage
      };
      rt = go.transform as RectTransform;
      rt.anchorMax = Vector2.one;
      rt.anchorMin = Vector2.zero;
      rt.offsetMax = Vector2.zero;
      rt.offsetMin = Vector2.zero;

      go = new GameObject("ButtonGlint", typeof(AnkiAnimateGlint), typeof(CozmoImage));
      go.transform.SetParent(button.transform, false);
      CozmoImage buttonGlint = go.GetComponent<CozmoImage>();
      buttonGlint.enabled = false;
      buttonGraphics[1] = new CozmoButton.AnkiButtonImage() {
        targetImage = buttonGlint
      };
      rt = go.transform as RectTransform;
      rt.anchorMax = Vector2.one;
      rt.anchorMin = Vector2.zero;
      rt.offsetMax = Vector2.zero;
      rt.offsetMin = Vector2.zero;
      AnkiAnimateGlint glint = go.GetComponent<AnkiAnimateGlint>();
      serializedObject.FindProperty("_GlintAnimator").objectReferenceValue = glint;
      SerializedObject glintSO = new SerializedObject(glint);
      glintSO.FindProperty("_MaskImage").objectReferenceValue = buttonGraphics[1].targetImage;
      glintSO.ApplyModifiedProperties();


      go = new GameObject("ButtonText", typeof(CozmoText));
      go.transform.SetParent(button.transform, false);
      rt = go.transform as RectTransform;
      rt.anchorMax = Vector2.one;
      rt.anchorMin = Vector2.zero;
      rt.offsetMax = Vector2.zero;
      rt.offsetMin = Vector2.zero;
      CozmoText ct = go.GetComponent<CozmoText>();
      ct.text = "Tap Me!";
      ct.LinkedComponentId = "CozmoButton_Primary_Text";
      ct.UpdateSkinnableElements(CozmoThemeSystemUtils.sInstance.GetCurrentThemeId(), CozmoThemeSystemUtils.sInstance.GetCurrentSkinId());
      serializedObject.FindProperty("_TextLabel").objectReferenceValue = ct;

      SerializedProperty buttonGraphicsArray = serializedObject.FindProperty("ButtonGraphics");
      buttonGraphicsArray.ClearArray();

      serializedObject.ApplyModifiedProperties();
      button.ButtonGraphics = buttonGraphics;
    }

  }

}

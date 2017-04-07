using Anki.Core.Editor.Components;
using Anki.Core.UI.Components;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace Cozmo.UI {
  [CustomEditor(typeof(CozmoButton))]
  [CanEditMultipleObjects]
  public class CozmoButtonEditor : AnkiButtonEditor {
    public override void OnInspectorGUI() {
      serializedObject.UpdateIfRequiredOrScript();

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

      //Get Target Script
      if (_ScriptTarget == null) {
        _ScriptTarget = (AnkiButton)target;
      }

      ThemeSystemEditorUtils.sInstance.DrawSkinningOptionsHeader();

      ThemeSystemEditorUtils.sInstance.DrawLinkedComponentIdDropdown(_ScriptTarget, ThemesJson.ComponentTypes.Button);

      if (string.IsNullOrEmpty(_ScriptTarget.LinkedComponentId) && _ScriptTarget.IsPreview) {
        ThemeSystemEditorUtils.sInstance.ResetPreview(_ScriptTarget);
      }

      if (!string.IsNullOrEmpty(_ScriptTarget.LinkedComponentId)) {
        //Force load the json that is on disk
        ThemesJson.LoadJson();
        ThemeSystemConfigJson.LoadJson();

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
  }

}

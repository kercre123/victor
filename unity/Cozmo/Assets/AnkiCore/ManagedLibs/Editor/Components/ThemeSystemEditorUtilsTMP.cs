using UnityEngine;
using UnityEditor;
using System.Collections;
using Anki.Core.Editor.Components;
using Anki.Core.UI.Components;
using System.Collections.Generic;

[InitializeOnLoad]
public class ThemeSystemEditorUtilsTMP : ThemeSystemEditorUtils {
  #region Initialize Contructor
  static ThemeSystemEditorUtilsTMP() {
    ThemeSystemEditorUtils.RegisterPreviewForAllChildrenAddOnDelegate(PreviewForAllChildrenTMP);
    ThemeSystemEditorUtils.RegisterResetPreviewForAllChildrenAddOnDelegate(ResetPreviewForAllChildrenTMP);
    ThemeSystemEditorUtils.RegisterFinishCreateNewThemeComponentForAllChildrenAddOnDelegate(FinishCreateNewThemeComponentForAllChildrenTMP);
  }
  #endregion

  #region Static Properties
  protected static ThemeSystemEditorUtilsTMP _sInstance = null;
  public static new ThemeSystemEditorUtilsTMP sInstance {
    get {
      if (_sInstance == null) {
        _sInstance = new ThemeSystemEditorUtilsTMP();
      }

      return _sInstance;
    }
  }
  #endregion

  #region Static AddOn Methods
  public static void PreviewForAllChildrenTMP() {
    if (Selection.activeTransform != null) {
      PreviewThemeIndex = ThemesJson.GetAllThemeIds().IndexOf(ThemesJson.CurrentlyLoadedInstance.CurrentThemeId);
      PreviewSkinIndex = ThemesJson.GetAllSkinIds().IndexOf(ThemesJson.CurrentlyLoadedInstance.CurrentSkinId);

      //Find all children of selection that contain TextMeshPro
      List<AnkiTextMeshPro> textComponents = new List<AnkiTextMeshPro>();
      textComponents.AddRange(Selection.activeTransform.GetComponentsInChildren<AnkiTextMeshPro>(true));

      for (int i = 0; i < textComponents.Count; i++) {
        sInstance.Preview(textComponents[i]);
      }

      Selection.activeGameObject.SetActive(!Selection.activeGameObject.activeSelf);
      Selection.activeGameObject.SetActive(!Selection.activeGameObject.activeSelf);
    }
    else {
      EditorUtility.DisplayDialog("Error", "Failed, selection empty.", "OK");
    }
  }

  public static void ResetPreviewForAllChildrenTMP() {
    if (Selection.activeTransform != null) {
      //Find all children of selection that contain TextMeshPro
      List<AnkiTextMeshPro> textComponents = new List<AnkiTextMeshPro>();
      textComponents.AddRange(Selection.activeTransform.GetComponentsInChildren<AnkiTextMeshPro>(true));

      for (int i = 0; i < textComponents.Count; i++) {
        sInstance.ResetPreview(textComponents[i]);
      }

      Selection.activeGameObject.SetActive(!Selection.activeGameObject.activeSelf);
      Selection.activeGameObject.SetActive(!Selection.activeGameObject.activeSelf);
    }
    else {
      EditorUtility.DisplayDialog("Error", "Failed, selection empty.", "OK");
    }
  }

  public static void FinishCreateNewThemeComponentForAllChildrenTMP(string baseComponentId) {
    //Find all children of selection that contain TextMeshPro(recursive so it includes off component)
    List<AnkiTextMeshPro> textComponents = new List<AnkiTextMeshPro>();
    textComponents.AddRange(Selection.activeTransform.GetComponentsInChildren<AnkiTextMeshPro>(true));

    //Check to make sure id isnt used by any component 
    string offenderID = null;
    for (int i = 0; i < textComponents.Count; i++) {
      if (ThemeSystemEditorUtilsTMP.sInstance.IdAlreadyExist(ThemeSystemEditorWindow.Categories.TextComponents, "TMP_" + baseComponentId + "_" + textComponents[i].name)) {
        offenderID = "TMP_" + baseComponentId + "_" + textComponents[i].name;
        break;
      }
    }

    if (string.IsNullOrEmpty(offenderID)) {
      //If we are good lets call CreateNewThemeComponent for each component we find
      for (int i = 0; i < textComponents.Count; i++) {
        ThemeSystemEditorUtilsTMP.sInstance.CreateNewThemeComponent("TMP_" + baseComponentId + "_" + textComponents[i].name, ThemesJson.ComponentTypes.Text);
        textComponents[i].LinkedComponentId = "TMP_" + baseComponentId + "_" + textComponents[i].name;
      }

      //Save
      ThemeSystemEditorUtilsTMP.sInstance.SaveJsonFromEditor(ThemesJson.CurrentlyLoadedInstance);

      int totalComponentCount = textComponents.Count;
      EditorUtility.DisplayDialog("Success", "Created " + totalComponentCount + " components succesfully for id: " + baseComponentId, "OK");
    }
    else {
      //Show error and quit
      EditorUtility.DisplayDialog("Error", "Failed, [" + offenderID + "] is already in use.", "OK");
    }
  }
  #endregion

  #region Preview Options
  public virtual void Preview(AnkiTextMeshPro scriptTarget) {
    Debug.Log("Preview [AnkiTextMeshProMeshPro]");

    if (!scriptTarget.IsPreview) {
      StoreSkinnableElementsInMeta(scriptTarget);
      scriptTarget.IsPreview = true;
    }
    else {
      ResetSkinnableElementsFromMeta(scriptTarget);
    }

    scriptTarget.UpdateSkinnableElements(ThemesJson.GetAllThemeIds()[PreviewThemeIndex], ThemesJson.GetAllSkinIds()[PreviewSkinIndex]);

    UpdatePreviewIds(scriptTarget);
  }

  public virtual void StoreSkinnableElementsInMeta(AnkiTextMeshPro scriptTarget) {
    scriptTarget.SerializedFaceColor = scriptTarget.color;
    scriptTarget.SerializedOutlineColor = scriptTarget.outlineColor;
    scriptTarget.SerializedOutlineThickness = scriptTarget.outlineWidth;
    scriptTarget.SerializeFontSize = (int)scriptTarget.fontSize;
  }

  public virtual void ResetPreview(AnkiTextMeshPro scriptTarget) {
    ResetSkinnableElementsFromMeta(scriptTarget);
    scriptTarget.IsPreview = false;
  }

  public virtual void ResetSkinnableElementsFromMeta(AnkiTextMeshPro scriptTarget) {
    scriptTarget.color = scriptTarget.SerializedFaceColor;
    scriptTarget.outlineColor = scriptTarget.SerializedOutlineColor;
    scriptTarget.outlineWidth = scriptTarget.SerializedOutlineThickness;
    scriptTarget.fontSize = scriptTarget.SerializeFontSize;
  }
  #endregion
}

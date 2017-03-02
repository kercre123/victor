using UnityEngine;
using UnityEditor;
using Anki.Core.UI.Components;

public class CozmoThemeSystemEditorWindow : Anki.Core.Editor.Components.ThemeSystemEditorWindow {
  #region Intilization
  [MenuItem("Window/Theme System")]
  protected static void InitializeWindowOverride() {
    _sWindow = EditorWindow.GetWindow<CozmoThemeSystemEditorWindow>();
    _sWindow.titleContent = new GUIContent("Theme System");
    _sWindow.position = new Rect(_sWindow.position.x, _sWindow.position.y, 1500f, 1000f);
    _sWindow.minSize = new Vector2(750f, 500f);
    _sWindow.Show();

    //Initialize variables
    ThemesJson.LoadJson();
    _sLastCurrentlySelectedThemeId = ThemesJson.CurrentlyLoadedInstance.CurrentThemeId;
    _sLastCurrentlySelectedSkinId = ThemesJson.CurrentlyLoadedInstance.CurrentSkinId;
  }
  #endregion

  #region Override Methods
  /*
  protected override void DrawInspectorComponentElementsUI() {
    base.DrawInspectorComponentElementsUI();

    string themeId = ThemesJson.GetAllThemeIds()[_InspectorComponentThemeIndex];
    string skinId = ThemesJson.GetAllSkinIds()[_InspectorComponentSkinIndex];
    ThemesJson.ThemeComponentObj component = ThemesJson.GetComponentById(themeId, skinId, _sCurrentlySelectedObj.Id);

    EditorGUI.BeginChangeCheck();

    if (component != null) {
      switch (_sCurrentlySelectedObj.Category) {
      case Categories.ButtonComponents:
        component.SkinShrinkOnClick = EditorGUILayout.Toggle("Skin Shrink On Click", component.SkinShrinkOnClick);
        component.ShrinkOnClick = EditorGUILayout.Toggle("Shrink On Click", component.ShrinkOnClick);
        component.SkinShrinkScale = EditorGUILayout.Toggle("Skin Shrink Scale", component.SkinShrinkScale);
        component.ShrinkScale = EditorGUILayout.FloatField("Shrink Scale", component.ShrinkScale);
        break;
      case Categories.TextComponents:
        component.SkinShadowEnabled = EditorGUILayout.Toggle("Skin Shadow Enabled", component.SkinShadowEnabled);
        component.ShadowEnabled = EditorGUILayout.Toggle("Shadow Enabled", component.ShadowEnabled);
        component.SkinShadowColor = EditorGUILayout.Toggle("Skin Shadow Color", component.SkinShadowColor);
        component.ShadowColor = EditorGUILayout.ColorField("Shadow Color", component.ShadowColor);
        component.SkinShadowDistance = EditorGUILayout.Toggle("Skin Shadow Distance", component.SkinShadowDistance);
        component.ShadowDistance.x = EditorGUILayout.FloatField("Shadow Distance X", component.ShadowDistance.x);
        component.ShadowDistance.y = EditorGUILayout.FloatField("Shadow Distance Y", component.ShadowDistance.y);
        component.SkinShadowUseGraphicAlpha = EditorGUILayout.Toggle("Skin Shadow Use Graphic Alpha", component.SkinShadowUseGraphicAlpha);
        component.ShadowUseGraphicAlpha = EditorGUILayout.Toggle("Shadow Use Graphic Alpha", component.ShadowUseGraphicAlpha);
        break;
      }
    }

    if (EditorGUI.EndChangeCheck()) {
      _sIsDirty = true;
    }
  }*/
  #endregion
}
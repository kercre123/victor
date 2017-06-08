using UnityEngine;
using UnityEditor;
using Anki.Core.UI.Components;
using System;
using Anki.Core.Editor.Components;

public class CozmoThemeSystemEditorWindow : Anki.Core.Editor.Components.ThemeSystemEditorWindow {

  private bool _ButtonImageFoldout = false;
  private bool[] _ButtonArrayFoldout = new bool[0];

  #region Intilization
  [MenuItem("Cozmo/Cozmo Theme System")]
  protected static void InitializeWindowOverride() {
    _sWindow = EditorWindow.GetWindow<CozmoThemeSystemEditorWindow>();
    _sWindow.titleContent = new GUIContent("Theme System");
    _sWindow.position = new Rect(_sWindow.position.x, _sWindow.position.y, 1400f, 800f);
    _sWindow.minSize = new Vector2(1400f, 800f);
    _sWindow.Show();

    //Initialize variables
    ThemesJson.LoadJson();

    _sLastCurrentlySelectedThemeId = ThemeSystemUtils.sInstance.GetCurrentThemeId();
    _sLastCurrentlySelectedSkinId = ThemeSystemUtils.sInstance.GetCurrentSkinId();
  }
  #endregion

  #region Override Methods

  protected override void SetupUIAreas() {
    float columnWidth = _sWindow.position.width * 0.25f;
    float headerHeight = 75f;
    float creationHeight = 110f;
    float selectionY = headerHeight + creationHeight;
    float selectionHeight = _sWindow.position.height - selectionY;

    _RuntimeOptionsSelectionAreaRect = new Rect(0,
      0,
      columnWidth,
      headerHeight);

    _ThemeSelectionAreaRect = new Rect(columnWidth,
      0,
      columnWidth,
      headerHeight);

    _SkinSelectionAreaRect = new Rect(columnWidth * 2,
      0,
      columnWidth,
      headerHeight);

    _CreationOptionsAreaRect = new Rect(0,
      headerHeight,
      columnWidth * 3,
      creationHeight);

    _ImageComponentSelectionAreaRect = new Rect(0,
      selectionY,
      columnWidth,
      selectionHeight);

    _TextComponentSelectionAreaRect = new Rect(0, 0, 0, 0);

    _TextMeshProComponentSelectionAreaRect = new Rect(columnWidth,
      selectionY,
      columnWidth,
      selectionHeight);

    _ButtonComponentSelectionAreaRect = new Rect(columnWidth * 2,
      selectionY,
      columnWidth,
      selectionHeight);

    _InspectorAreaRect = new Rect(_sWindow.position.width - columnWidth,
      0,
      columnWidth,
      _sWindow.position.height);
  }

  protected override void DrawInspectorButtonComponentUI(ThemesJson.ThemeComponentObj component) {
    component.SkinButtonTransition = EditorGUILayout.Toggle("Skin Text Transition Colors", component.SkinButtonTransition);
    component.ButtonTransition.ColorBlock.normalColor = EditorGUILayout.ColorField("Normal Text Color", component.ButtonTransition.ColorBlock.normalColor);
    component.ButtonTransition.ColorBlock.pressedColor = EditorGUILayout.ColorField("Pressed Text Color", component.ButtonTransition.ColorBlock.pressedColor);
    component.ButtonTransition.ColorBlock.disabledColor = EditorGUILayout.ColorField("Disabled Text Color", component.ButtonTransition.ColorBlock.disabledColor);

    component.SkinButtonImageArray = EditorGUILayout.Toggle("Skin Button Image Array", component.SkinButtonImageArray);
    _ButtonImageFoldout = EditorGUILayout.Foldout(_ButtonImageFoldout, "Button Image Array");
    if (_ButtonImageFoldout) {

      // deal with array size
      int buttonArraySize = EditorGUILayout.IntField("Button Array Size", component.ButtonImageArray.Length);
      if (buttonArraySize != component.ButtonImageArray.Length) {
        Anki.Core.UI.Components.ThemesJson.ButtonImageObj[] oldArray = component.ButtonImageArray;
        component.ButtonImageArray = new ThemesJson.ButtonImageObj[buttonArraySize];
        int i = 0;
        for (; i < Mathf.Min(oldArray.Length, buttonArraySize); ++i) {
          component.ButtonImageArray[i] = oldArray[i];
        }

        for (; i < buttonArraySize; ++i) {
          component.ButtonImageArray[i] = new Anki.Core.UI.Components.ThemesJson.ButtonImageObj();
        }

      }

      // updates fold bool tracking for the elements
      if (buttonArraySize != _ButtonArrayFoldout.Length) {
        bool[] oldArray = _ButtonArrayFoldout;
        _ButtonArrayFoldout = new bool[buttonArraySize];

        int i = 0;
        for (; i < Mathf.Min(oldArray.Length, buttonArraySize); ++i) {
          _ButtonArrayFoldout[i] = oldArray[i];
        }
        for (; i < buttonArraySize; ++i) {
          _ButtonArrayFoldout[i] = false;
        }
      }

      // draws the elements
      for (int i = 0; i < buttonArraySize; ++i) {
        _ButtonArrayFoldout[i] = EditorGUILayout.Foldout(_ButtonArrayFoldout[i], "Element " + i);
        if (_ButtonArrayFoldout[i]) {
          component.ButtonImageArray[i].PressedSpriteColor = EditorGUILayout.ColorField("Pressed Color", component.ButtonImageArray[i].PressedSpriteColor);

          Sprite sprite = null;

          if (!string.IsNullOrEmpty(component.ButtonImageArray[i].PressedSpriteResourceKey)) {
            sprite = ThemeSystemUtils.sInstance.LoadSprite(component.ButtonImageArray[i].PressedSpriteResourceKey);
          }

          sprite = (Sprite)EditorGUILayout.ObjectField("Pressed Sprite", sprite, typeof(Sprite), true);
          if (sprite != null) {
            component.ButtonImageArray[i].PressedSpriteResourceKey = ThemeSystemEditorUtils.sInstance.GetAssetResourcesPath(sprite);
          }

          component.ButtonImageArray[i].DisabledSpriteColor = EditorGUILayout.ColorField("Disabled Color", component.ButtonImageArray[i].DisabledSpriteColor);

          sprite = null;

          if (!string.IsNullOrEmpty(component.ButtonImageArray[i].DisabledSpriteResourceKey)) {
            sprite = ThemeSystemUtils.sInstance.LoadSprite(component.ButtonImageArray[i].DisabledSpriteResourceKey);
          }

          sprite = (Sprite)EditorGUILayout.ObjectField("Disabled Sprite", sprite, typeof(Sprite), true);
          if (sprite != null) {
            component.ButtonImageArray[i].DisabledSpriteResourceKey = ThemeSystemEditorUtils.sInstance.GetAssetResourcesPath(sprite);
          }

          EditorGUILayout.Space();
        }


      }
    }

  }

  protected override void DrawInspectorTextMeshProComponentUI(ThemesJson.ThemeComponentObj component) {
    base.DrawInspectorTextMeshProComponentUI(component);
    component.SkinFontStyle = EditorGUILayout.Toggle("Skin Font Style", component.SkinFontStyle);
    component.FontStyle = System.Convert.ToInt32(EditorGUILayout.EnumPopup("Font Style", (TMPro.FontStyles)component.FontStyle));

    component.SkinTextAlignment = EditorGUILayout.Toggle("Skin Text Alignment", component.SkinTextAlignment);

    #region TextAlignment
    EditorGUI.BeginChangeCheck();

    Rect rect = EditorGUILayout.GetControlRect(false, 20);
    GUIStyle btn = new GUIStyle(GUI.skin.button);
    btn.margin = new RectOffset(1, 1, 1, 1);
    btn.padding = new RectOffset(1, 1, 1, 0);

    int selAlignGrid_A = TMPro.EditorUtilities.TMP_EditorUtility.GetHorizontalAlignmentGridValue(component.TextAlignment);
    int selAlignGrid_B = TMPro.EditorUtilities.TMP_EditorUtility.GetVerticalAlignmentGridValue(component.TextAlignment);

    GUI.Label(new Rect(rect.x, rect.y + 2, 100, rect.height), "Alignment");
    float columnB = EditorGUIUtility.labelWidth;

    TMPro.EditorUtilities.TMP_UIStyleManager.GetUIStyles();
    selAlignGrid_A = GUI.SelectionGrid(new Rect(columnB, rect.y, 23 * 6, rect.height), selAlignGrid_A, TMPro.EditorUtilities.TMP_UIStyleManager.alignContent_A, 6, btn);
    selAlignGrid_B = GUI.SelectionGrid(new Rect(columnB, rect.y + 25, 23 * 6, rect.height), selAlignGrid_B, TMPro.EditorUtilities.TMP_UIStyleManager.alignContent_B, 6, btn);

    if (EditorGUI.EndChangeCheck()) {
      int value = (0x1 << selAlignGrid_A) | (0x100 << selAlignGrid_B);
      component.TextAlignment = value;
    }

    EditorGUILayout.Space();
    EditorGUILayout.Space();
    EditorGUILayout.Space();
    EditorGUILayout.Space();

    #endregion

    component.SkinTextOverflow = EditorGUILayout.Toggle("Skin Text Overflow", component.SkinTextOverflow);

    TMPro.TextOverflowModes textOverflowMode = (TMPro.TextOverflowModes)component.TextOverflow;
    component.TextOverflow = System.Convert.ToInt32((TMPro.TextOverflowModes)EditorGUILayout.EnumPopup("Text Overflow", textOverflowMode));

    component.SkinAutoSizing = EditorGUILayout.Toggle("Skin AutoSizing", component.SkinAutoSizing);
    component.EnableAutoSizing = EditorGUILayout.Toggle("Enable AutoSizing", component.EnableAutoSizing);

    if (component.EnableAutoSizing) {
      component.AutoSizingMin = EditorGUILayout.FloatField("Auto Size Min", component.AutoSizingMin);
      component.AutoSizingMax = EditorGUILayout.FloatField("Auto Size Max", component.AutoSizingMax);
    }
  }

  #endregion
}
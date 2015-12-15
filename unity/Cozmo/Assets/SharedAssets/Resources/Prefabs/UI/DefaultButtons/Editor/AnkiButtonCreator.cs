using UnityEditor;
using UnityEngine;
using Anki.UI;
using Cozmo.UI;

public static class AnkiButtonCreator {

  [MenuItem("Cozmo/UI/Create Green Button %g")]
  private static void CreateGreenButtonUnderSelection() {
    AnkiButton button = CreateButton(Selection.activeTransform.gameObject);
    SetButtonValuesGreen(button);
    SelectObject(button.gameObject);
  }

  [MenuItem("Cozmo/UI/Create Green Button %g", true)]
  private static bool ValidateCreateGreenButtonUnderSelection() {
    return SelectionIsRectTransform();
  }

  [MenuItem("GameObject/Cozmo/Green Button", false, 10)]
  private static void ContextCreateGreenButton(MenuCommand command) {
    AnkiButton button = CreateButton(command.context as GameObject);
    SetButtonValuesGreen(button);
    SelectObject(button.gameObject);
  }

  [MenuItem("Cozmo/UI/Create Red Button %r")]
  private static void CreateRedButtonUnderSelection() {
    AnkiButton button = CreateButton(Selection.activeTransform.gameObject);
    SetButtonValuesRed(button);
    SelectObject(button.gameObject);
  }

  [MenuItem("Cozmo/UI/Create Red Button %r", true)]
  private static bool ValidateCreateRedButtonUnderSelection() {
    return SelectionIsRectTransform();
  }

  [MenuItem("GameObject/Cozmo/Red Button", false, 10)]
  private static void ContextCreateRedButton(MenuCommand command) {
    AnkiButton button = CreateButton(command.context as GameObject);
    SetButtonValuesRed(button);
    SelectObject(button.gameObject);
  }

  private static AnkiButton CreateButton(GameObject parent) {
    GameObject button = GameObject.Instantiate(UIPrefabHolder.Instance.DefaultButtonPrefab.gameObject);
    button.transform.SetParent(parent.transform, false);
    return button.GetComponent<AnkiButton>();
  }

  private static void SelectObject(GameObject go) {
    // Register the creation in the undo system
    Undo.RegisterCreatedObjectUndo(go, "Create " + go.name);
    Selection.activeObject = go;
  }

  private static bool SelectionIsRectTransform() {
    // Return false if no transform is selected.
    return Selection.activeTransform != null && Selection.activeTransform is RectTransform;
  }

  private static void SetButtonValuesGreen(AnkiButton target) {
    target.gameObject.name = "Green Button";
    SetTextValues(target);
    SetButtonDisabledValues(target);

    // Slot 0 is the background.
    target.ButtonGraphics[0].enabledColor = Color.white;
    target.ButtonGraphics[0].pressedColor = Color.white;
    // Slot 1 is the gradient.
    target.ButtonGraphics[1].enabledColor = new Color32(131, 194, 11, 255);
    target.ButtonGraphics[1].pressedColor = new Color32(54, 124, 2, 255);
    // Slot 2 is the deco
    target.ButtonGraphics[2].enabledColor = new Color32(67, 143, 80, 255);
    target.ButtonGraphics[2].pressedColor = target.ButtonGraphics[2].enabledColor;
  }

  private static void SetButtonValuesRed(AnkiButton target) {
    target.gameObject.name = "Red Button";
    SetTextValues(target);
    SetButtonDisabledValues(target);

    // Slot 0 is the background.
    target.ButtonGraphics[0].enabledColor = Color.white;
    target.ButtonGraphics[0].pressedColor = Color.white;
    // Slot 1 is the gradient.
    target.ButtonGraphics[1].enabledColor = new Color32(226, 24, 24, 255);
    target.ButtonGraphics[1].pressedColor = new Color32(146, 1, 1, 255);
    // Slot 2 is the deco
    target.ButtonGraphics[2].enabledColor = new Color32(121, 50, 67, 255);
    target.ButtonGraphics[2].pressedColor = target.ButtonGraphics[2].enabledColor;
  }

  private static void SetTextValues(AnkiButton target) {
    target.TextEnabledColor = new Color32(248, 254, 255, 255);
    target.TextPressedColor = target.TextEnabledColor;
    target.TextDisabledColor = new Color32(139, 160, 175, 255);
    target.TextDisabledColor = new Color32(139, 160, 175, 255);
  }

  private static void SetButtonDisabledValues(AnkiButton target) {
    // Slot 0 is the background.
    target.ButtonGraphics[0].disabledColor = Color.white;
    // Slot 1 is the gradient.
    target.ButtonGraphics[1].disabledColor = new Color32(54, 79, 98, 255);
    // Slot 2 is the deco
    target.ButtonGraphics[2].disabledColor = new Color32(55, 75, 100, 255);
  }
}

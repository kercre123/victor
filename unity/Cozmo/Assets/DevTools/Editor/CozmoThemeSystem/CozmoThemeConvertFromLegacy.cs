using UnityEditor;
using UnityEngine;
using Anki.UI;
using UnityEngine.UI;
using Cozmo.UI;

public class CozmoThemeConvertFromLegacy {
  [MenuItem("Cozmo/UI Tools/Convert Legacy")]
  public static void ConvertLegacy() {
    foreach (GameObject obj in Selection.gameObjects) {
      var button = obj.GetComponent<CozmoButtonLegacy>();
      if (button != null) {
        Debug.LogWarning("Can't convert legacy buttons yet!", obj);
        continue;
      }
      var text = obj.GetComponent<Text>();
      if (text != null) {
        ConvertLegacyText(obj, text);
        continue;
      }
      var image = obj.GetComponent<Image>();
      if (image != null) {
        ConvertLegacyImage(obj, image);
        continue;
      }
    }
  }

  public static void ConvertLegacyImage(GameObject obj, Image image) {
    // the undo stuff isn't well documented, this grouping attempt is just a guess
    Undo.IncrementCurrentGroup();
    Undo.SetCurrentGroupName("Convert Legacy Image");
    Sprite oldSprite = image.sprite;
    bool oldPreserveAspect = image.preserveAspect;
    Color oldColor = image.color;
    Image.Type oldType = image.type;
    bool oldFillCenter = image.fillCenter;

    Undo.DestroyObjectImmediate(image);
    CozmoImage newImage = Undo.AddComponent<CozmoImage>(obj);
    newImage.sprite = oldSprite;
    newImage.preserveAspect = oldPreserveAspect;
    newImage.color = oldColor;
    newImage.type = oldType;
    newImage.fillCenter = oldFillCenter;
  }

  public static void ConvertLegacyText(GameObject obj, Text text) {
    // save off old fields: rect transform values?
    Undo.IncrementCurrentGroup();
    Undo.SetCurrentGroupName("Convert Legacy Text");
    Undo.RegisterCompleteObjectUndo(obj, "Convert Legacy Text");

    RectTransform rt = obj.transform as RectTransform;
    Vector2 size = rt.sizeDelta;
    Vector2 pos = rt.anchoredPosition;
    Vector2 anchorMin = rt.anchorMin;
    Vector2 anchorMax = rt.anchorMax;

    string oldText = text.text;
    float oldFontSize = text.fontSize;
    TextAnchor oldAnchor = text.alignment;
    Color oldColor = text.color;
    float oldLineSpacing = text.lineSpacing;
    bool oldRichText = text.supportRichText;

    Undo.DestroyObjectImmediate(text);
    CozmoText newText = Undo.AddComponent<CozmoText>(obj);

    newText.SetText(oldText);
    newText.fontSize = oldFontSize;
    newText.alignment = ConvertAlignment(oldAnchor);
    newText.color = oldColor;
    newText.lineSpacing = oldLineSpacing;
    newText.richText = oldRichText;

    rt.anchorMax = anchorMax;
    rt.anchorMin = anchorMin;
    rt.anchoredPosition = pos;
    rt.sizeDelta = size;
  }

  private static TMPro.TextAlignmentOptions ConvertAlignment(TextAnchor anchor) {
    switch (anchor) {
    case TextAnchor.LowerCenter: return TMPro.TextAlignmentOptions.Bottom;
    case TextAnchor.LowerLeft: return TMPro.TextAlignmentOptions.BottomLeft;
    case TextAnchor.LowerRight: return TMPro.TextAlignmentOptions.BottomRight;
    case TextAnchor.MiddleCenter: return TMPro.TextAlignmentOptions.Center;
    case TextAnchor.MiddleLeft: return TMPro.TextAlignmentOptions.Left;
    case TextAnchor.MiddleRight: return TMPro.TextAlignmentOptions.Right;
    case TextAnchor.UpperCenter: return TMPro.TextAlignmentOptions.Top;
    case TextAnchor.UpperLeft: return TMPro.TextAlignmentOptions.TopLeft;
    case TextAnchor.UpperRight: return TMPro.TextAlignmentOptions.TopRight;
    }
    return TMPro.TextAlignmentOptions.TopLeft;
  }
}


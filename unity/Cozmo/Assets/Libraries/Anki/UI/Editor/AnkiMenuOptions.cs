using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using UnityEditor.UI;
using UnityEditor;
using Anki.UI;

namespace Anki.Editor.UI {
  /// <summary>
  /// This script adds the UI menu options to the Unity Editor.
  /// </summary>
  
  static internal class AnkiMenuOptions {
    private const string kUILayerName = "UI";
    private const float  kWidth = 522f;
    private const float  kHeight = 136f;
    private const float  _ResolutionWidth = 1920f;
    private const float  _ResolutionHeight = 1080f;
    private const float  _MatchHeightWidthRatio = 0f;
    
    private static Vector2 s_ElementSize = new Vector2(kWidth, kHeight);
    private static Color   s_PanelColor = new Color(1f, 1f, 1f, 1f);
    private static int     s_FontSize = 78;
    private static Vector2 s_ZeroVector = new Vector2(0f, 0f);
    private static Vector2 s_OneVector = new Vector2(1f, 1f);
    private static Vector2 s_TextAnchorMin = new Vector2(0.06f, 0.3f);
    private static Vector2 s_TextAnchorMax = new Vector2(0.94f, 0.7f);
    
    private static void SetPositionVisibleinSceneView(RectTransform canvasRTransform, RectTransform itemTransform) {
      // Find the best scene view
      SceneView sceneView = SceneView.lastActiveSceneView;
      if (sceneView == null && SceneView.sceneViews.Count > 0)
        sceneView = SceneView.sceneViews[0] as SceneView;
      
      // Couldn't find a SceneView. Don't set position.
      if (sceneView == null || sceneView.camera == null)
        return;
      
      // Create world space Plane from canvas position.
      Vector2 localPlanePosition;
      Camera camera = sceneView.camera;
      Vector3 position = Vector3.zero;
      if (RectTransformUtility.ScreenPointToLocalPointInRectangle(canvasRTransform, new Vector2(camera.pixelWidth / 2, camera.pixelHeight / 2), camera, out localPlanePosition)) {
        // Adjust for canvas pivot
        localPlanePosition.x = localPlanePosition.x + canvasRTransform.sizeDelta.x * canvasRTransform.pivot.x;
        localPlanePosition.y = localPlanePosition.y + canvasRTransform.sizeDelta.y * canvasRTransform.pivot.y;
        
        localPlanePosition.x = Mathf.Clamp(localPlanePosition.x, 0, canvasRTransform.sizeDelta.x);
        localPlanePosition.y = Mathf.Clamp(localPlanePosition.y, 0, canvasRTransform.sizeDelta.y);
        
        // Adjust for anchoring
        position.x = localPlanePosition.x - canvasRTransform.sizeDelta.x * itemTransform.anchorMin.x;
        position.y = localPlanePosition.y - canvasRTransform.sizeDelta.y * itemTransform.anchorMin.y;
        
        Vector3 minLocalPosition;
        minLocalPosition.x = canvasRTransform.sizeDelta.x * (0 - canvasRTransform.pivot.x) + itemTransform.sizeDelta.x * itemTransform.pivot.x;
        minLocalPosition.y = canvasRTransform.sizeDelta.y * (0 - canvasRTransform.pivot.y) + itemTransform.sizeDelta.y * itemTransform.pivot.y;
        
        Vector3 maxLocalPosition;
        maxLocalPosition.x = canvasRTransform.sizeDelta.x * (1 - canvasRTransform.pivot.x) - itemTransform.sizeDelta.x * itemTransform.pivot.x;
        maxLocalPosition.y = canvasRTransform.sizeDelta.y * (1 - canvasRTransform.pivot.y) - itemTransform.sizeDelta.y * itemTransform.pivot.y;
        
        position.x = Mathf.Clamp(position.x, minLocalPosition.x, maxLocalPosition.x);
        position.y = Mathf.Clamp(position.y, minLocalPosition.y, maxLocalPosition.y);
      }
      
      itemTransform.anchoredPosition = position;
      itemTransform.localRotation = Quaternion.identity;
      itemTransform.localScale = Vector3.one;
    }
    
    private static GameObject CreateUIElementRoot(string name, MenuCommand menuCommand, Vector2 size) {
      GameObject parent = menuCommand.context as GameObject;
      if (parent == null || parent.GetComponentInParent<Canvas>() == null) {
        parent = GetOrCreateCanvasGameObject();
      }
      GameObject child = new GameObject(name);
      
      Undo.RegisterCreatedObjectUndo(child, "Create " + name);
      Undo.SetTransformParent(child.transform, parent.transform, "Parent " + child.name);
      GameObjectUtility.SetParentAndAlign(child, parent);
      
      RectTransform rectTransform = child.AddComponent<RectTransform>();
      rectTransform.sizeDelta = size;
      if (parent != menuCommand.context) { // not a context click, so center in sceneview
        SetPositionVisibleinSceneView(parent.GetComponent<RectTransform>(), rectTransform);
      }
      Selection.activeGameObject = child;
      return child;
    }
    
    [MenuItem("GameObject/Anki/Window", false, 2000)]
    static public void AddAnkiWindowFromMenu(MenuCommand menuCommand) {
      GameObject windowRoot = CreateUIElementRoot("AnkiWindow", menuCommand, s_ElementSize);
      
      // Set RectTransform to stretch
      RectTransform windowRect = windowRoot.GetComponent<RectTransform>();
      windowRect.anchorMin = new Vector2(0.42f, 0.029f);
      windowRect.anchorMax = new Vector2(0.7f, 0.2f);
      windowRect.anchoredPosition = Vector2.zero;
      
      Image bgImage = windowRoot.AddComponent<Image>();
      bgImage.sprite = Anki.AppResources.SpriteCache.LoadSprite("Components/Button/ui_button_primary_orange_background");
      SetImageDefaults(bgImage);
      
      //Create Mask Panel
      GameObject maskPanel = CreateUIObject("MaskPanel", windowRoot);
      
      Mask mask = maskPanel.AddComponent<Mask>();
      mask.showMaskGraphic = false;
      
      Image maskImage = maskPanel.AddComponent<Image>();
      maskImage.sprite = Anki.AppResources.SpriteCache.LoadSprite("Components/Button/ui_button_primary_mask");
      SetImageDefaults(maskImage);
      
      //Create Masked Panel
      GameObject maskedPanel = CreateUIObject("MaskedBackground", maskPanel);
      
      Image maskedImage = maskedPanel.AddComponent<Image>();
      maskedImage.sprite = Anki.AppResources.SpriteCache.LoadSprite("Components/Button/ui_button_primary_orange_arrows");
      SetImageDefaults(maskedImage);
      
      //Create Content Panel
      GameObject contentPanel = CreateUIObject("ContentPanel", windowRoot);
      
      Anki.UI.AnkiWindow windowComp = windowRoot.AddComponent<Anki.UI.AnkiWindow>();
      windowComp.SetBGRef = bgImage;
      windowComp.SetContentRef = contentPanel;
      windowComp.SetMaskRef = mask;
      windowComp.SetMaskBgRef = maskedImage;
    }
    
    [MenuItem("GameObject/Anki/OrangeButton", false, 2000)]
    static public void AddAnkiOrangeButtonFromMenu(MenuCommand menuCommand) {
      GameObject buttonRoot = CreateUIElementRoot("AnkiButton", menuCommand, s_ElementSize);
      
      // Set RectTransform to stretch
      RectTransform windowRect = buttonRoot.GetComponent<RectTransform>();
      windowRect.anchorMin = new Vector2(0.42f, 0.029f);
      windowRect.anchorMax = new Vector2(0.7f, 0.2f);
      windowRect.sizeDelta = Vector2.zero;
      
      Image bgImage = buttonRoot.AddComponent<Image>();
      bgImage.sprite = Anki.AppResources.SpriteCache.LoadSprite("Components/Button/ui_button_primary_orange");
      SetImageDefaults(bgImage);
      
      //Create Mask Panel
      GameObject maskPanel = CreateUIObject("MaskPanel", buttonRoot);
      
      Mask mask = maskPanel.AddComponent<Mask>();
      mask.showMaskGraphic = false;
      
      Image maskImage = maskPanel.AddComponent<Image>();
      maskImage.sprite = Anki.AppResources.SpriteCache.LoadSprite("Components/Button/ui_button_primary_mask");
      SetImageDefaults(maskImage);
      
      //Create Masked Panel
      GameObject maskedPanel = CreateUIObject("MaskedBackground", maskPanel);
      
      Image maskedImage = maskedPanel.AddComponent<Image>();
      SetImageDefaults(maskedImage);
      maskedImage.enabled = false;
      
      //Create Content Panel
      GameObject contentPanel = CreateUIObject("ContentPanel", buttonRoot);
      
      //Create Text Panel
      GameObject textPanel = CreateUIObject("ButtonText", contentPanel);

      RectTransform textRect = textPanel.GetComponent<RectTransform>();
      textRect.anchorMin = s_TextAnchorMin;
      textRect.anchorMax = s_TextAnchorMax;
      textRect.anchoredPosition = Vector2.zero;

      AnkiTextLabel text = textPanel.AddComponent<AnkiTextLabel>();
      text.text = "Button Text".ToUpper();
      text.font = Resources.Load<Font>("fonts/DriveSans-Medium") as Font;
      text.fontSize = s_FontSize;
      text.alignment = TextAnchor.MiddleCenter;
      text.color = new Color32(255, 241, 195, 255);
      text.resizeTextForBestFit = true;
      text.resizeTextMinSize = 10;
      text.resizeTextMaxSize = s_FontSize;

      Shadow shadowComp = textPanel.AddComponent<Shadow>();
      shadowComp.effectDistance = new Vector2(4, -4);
      shadowComp.effectColor = new Color32(0, 0, 0, 96);
      
      Anki.UI.AnkiWindow windowComp = buttonRoot.AddComponent<Anki.UI.AnkiWindow>();
      windowComp.SetBGRef = bgImage;
      windowComp.SetContentRef = contentPanel;
      windowComp.SetMaskRef = mask;
      windowComp.SetMaskBgRef = maskedImage;
      
      Anki.UI.AnkiButton buttonComp = buttonRoot.AddComponent<Anki.UI.AnkiButton>();
      buttonComp.SetTextRef = text;
      buttonComp.targetGraphic = bgImage;
    }

    [MenuItem("GameObject/Anki/BlueButton", false, 2000)]
    static public void AddAnkiBlueButtonFromMenu(MenuCommand menuCommand) {
      GameObject buttonRoot = CreateUIElementRoot("AnkiButton", menuCommand, s_ElementSize);
      
      // Set RectTransform to stretch
      RectTransform windowRect = buttonRoot.GetComponent<RectTransform>();
      windowRect.anchorMin = new Vector2(0.42f, 0.029f);
      windowRect.anchorMax = new Vector2(0.7f, 0.2f);
      windowRect.sizeDelta = Vector2.zero;
      
      Image bgImage = buttonRoot.AddComponent<Image>();
      bgImage.sprite = Anki.AppResources.SpriteCache.LoadSprite("Components/Button/ui_button_secondary_blue");
      SetImageDefaults(bgImage);
      
      //Create Mask Panel
      GameObject maskPanel = CreateUIObject("MaskPanel", buttonRoot);
      
      Mask mask = maskPanel.AddComponent<Mask>();
      mask.showMaskGraphic = false;
      
      Image maskImage = maskPanel.AddComponent<Image>();
      maskImage.sprite = Anki.AppResources.SpriteCache.LoadSprite("Components/Button/ui_button_primary_mask");
      SetImageDefaults(maskImage);
      
      //Create Masked Panel
      GameObject maskedPanel = CreateUIObject("MaskedBackground", maskPanel);
      
      Image maskedImage = maskedPanel.AddComponent<Image>();
      SetImageDefaults(maskedImage);
      maskedImage.enabled = false;
      
      //Create Content Panel
      GameObject contentPanel = CreateUIObject("ContentPanel", buttonRoot);
      
      //Create Text Panel
      GameObject textPanel = CreateUIObject("ButtonText", contentPanel);
      
      RectTransform textRect = textPanel.GetComponent<RectTransform>();
      textRect.anchorMin = s_TextAnchorMin;
      textRect.anchorMax = s_TextAnchorMax;
      textRect.anchoredPosition = Vector2.zero;
      
      AnkiTextLabel text = textPanel.AddComponent<AnkiTextLabel>();
      text.text = "Button Text".ToUpper();
      text.font = Resources.Load<Font>("fonts/DriveSans-Medium") as Font;
      text.fontSize = s_FontSize;
      text.alignment = TextAnchor.MiddleCenter;
      text.color = new Color32(180, 243, 255, 255);
      text.resizeTextForBestFit = true;
      text.resizeTextMinSize = 10;
      text.resizeTextMaxSize = s_FontSize;
      
      Shadow shadowComp = textPanel.AddComponent<Shadow>();
      shadowComp.effectDistance = new Vector2(4, -4);
      shadowComp.effectColor = new Color32(0, 0, 0, 96);
      
      Anki.UI.AnkiWindow windowComp = buttonRoot.AddComponent<Anki.UI.AnkiWindow>();
      windowComp.SetBGRef = bgImage;
      windowComp.SetContentRef = contentPanel;
      windowComp.SetMaskRef = mask;
      windowComp.SetMaskBgRef = maskedImage;
      
      Anki.UI.AnkiButton buttonComp = buttonRoot.AddComponent<Anki.UI.AnkiButton>();
      buttonComp.SetTextRef = text;
      buttonComp.targetGraphic = bgImage;
    }

    [MenuItem("GameObject/Anki/AnkiTextLabel", false, 2002)]
    static public void AddAnkiText(MenuCommand menuCommand)
    {
      GameObject go = CreateUIElementRoot("AnkiText", menuCommand, s_ElementSize);

      AnkiTextLabel text = go.AddComponent<AnkiTextLabel>();
      text.text = "New Text";
      text.font = Resources.Load<Font>("fonts/DriveSans-Medium") as Font;
      text.fontSize = s_FontSize;
      text.alignment = TextAnchor.MiddleCenter;
      text.color = new Color32(180, 243, 255, 255);
      text.resizeTextForBestFit = true;
      text.resizeTextMinSize = 10;
      text.resizeTextMaxSize = s_FontSize;
    }
    
    static Image SetImageDefaults(Image img) {
      img.type = Image.Type.Sliced;
      img.color = s_PanelColor;
      return img;
    }
    
    static GameObject CreateUIObject(string name, GameObject parent) {
      GameObject go = new GameObject(name);
      RectTransform rect = go.AddComponent<RectTransform>();
      rect.anchorMin = s_ZeroVector;
      rect.anchorMax = s_OneVector;
      rect.anchoredPosition = s_ZeroVector;
      rect.sizeDelta = s_ZeroVector;
      GameObjectUtility.SetParentAndAlign(go, parent);
      return go;
    }
    
    private static void SetDefaultColorTransitionValues(Selectable slider) {
      ColorBlock colors = slider.colors;
      colors.highlightedColor = new Color(0.882f, 0.882f, 0.882f);
      colors.pressedColor = new Color(0.698f, 0.698f, 0.698f);
      colors.disabledColor = new Color(0.521f, 0.521f, 0.521f);
    }
    
    [MenuItem("GameObject/Anki/CustomCanvas", false, 2000)]
    static public GameObject CreateNewUI() {
      // Root for the UI
      var root = new GameObject("Canvas");
      root.layer = LayerMask.NameToLayer(kUILayerName);
      Canvas canvas = root.AddComponent<Canvas>();
      canvas.renderMode = RenderMode.ScreenSpaceOverlay;
      CanvasScaler scaler = root.AddComponent<CanvasScaler>();
      scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
      scaler.referenceResolution = new Vector2(_ResolutionWidth, _ResolutionHeight);
      scaler.matchWidthOrHeight = _MatchHeightWidthRatio;
      root.AddComponent<GraphicRaycaster>();
      Undo.RegisterCreatedObjectUndo(root, "Create " + root.name);
      
      // if there is no event system add one...
      CreateEventSystem(false);
      return root;
    }
    
    private static void CreateEventSystem(bool select) {
      CreateEventSystem(select, null);
    }
    
    private static void CreateEventSystem(bool select, GameObject parent) {
      var esys = Object.FindObjectOfType<EventSystem>();
      if (esys == null) {
        var eventSystem = new GameObject("EventSystem");
        GameObjectUtility.SetParentAndAlign(eventSystem, parent);
        esys = eventSystem.AddComponent<EventSystem>();
        eventSystem.AddComponent<StandaloneInputModule>();
        eventSystem.AddComponent<TouchInputModule>();
        
        Undo.RegisterCreatedObjectUndo(eventSystem, "Create " + eventSystem.name);
      }
      
      if (select && esys != null) {
        Selection.activeGameObject = esys.gameObject;
      }
    }
    
    // Helper function that returns a Canvas GameObject; preferably a parent of the selection, or other existing Canvas.
    static public GameObject GetOrCreateCanvasGameObject() {
      GameObject selectedGo = Selection.activeGameObject;
      
      // Try to find a gameobject that is the selected GO or one if its parents.
      Canvas canvas = (selectedGo != null) ? selectedGo.GetComponentInParent<Canvas>() : null;
      if (canvas != null && canvas.gameObject.activeInHierarchy)
        return canvas.gameObject;
      
      // No canvas in selection or its parents? Then use just any canvas..
      canvas = Object.FindObjectOfType(typeof(Canvas)) as Canvas;
      if (canvas != null && canvas.gameObject.activeInHierarchy)
        return canvas.gameObject;
      
      // No canvas in the scene at all? Then create a new one.
      return AnkiMenuOptions.CreateNewUI();
    }
  }
}

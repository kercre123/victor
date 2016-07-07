using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections.Generic;
using DG.Tweening;
using Cozmo.UI;
using DataPersistence;

public class UIManager : MonoBehaviour {

  private static UIManager _Instance;

  public static UIManager Instance {
    get {
      if (_Instance == null) {
        DAS.Error("UIManager.NullInstance", "Do not access UIManager until start: " + System.Environment.StackTrace);
      }
      return _Instance;
    }
    private set {
      if (_Instance != null) {
        DAS.Error("UIManager.DuplicateInstance", "UIManager Instance already exists");
      }
      _Instance = value;
    }
  }

  public static Vector4 GetAtlasUVs(Sprite graphic) {
    float textureAtlasPixelWidth = graphic.texture.width;
    float xMinNormalizedUV = graphic.textureRect.xMin / textureAtlasPixelWidth;
    float xMaxNormalizedUV = graphic.textureRect.xMax / textureAtlasPixelWidth;

    float textureAtlasPixelHeight = graphic.texture.height;
    float yMinNormalizedUV = graphic.textureRect.yMin / textureAtlasPixelHeight;
    float yMaxNormalizedUV = graphic.textureRect.yMax / textureAtlasPixelHeight;

    return new Vector4(xMinNormalizedUV,
      yMinNormalizedUV,
      xMaxNormalizedUV - xMinNormalizedUV,
      yMaxNormalizedUV - yMinNormalizedUV);
  }

  [SerializeField]
  private Canvas _HorizontalCanvas;

  public Camera MainCamera;

  public EventSystem EventSystemScript;

  [SerializeField]
  private GameObject _TouchCatcherPrefab;

  public GameObject TouchCatcherPrefab {
    get { return _TouchCatcherPrefab; }
  }
  private TouchCatcher _TouchCatcherInstance;
  public TouchCatcher TouchCatcher { get { return _TouchCatcherInstance != null && _TouchCatcherInstance.isActiveAndEnabled ? _TouchCatcherInstance : null; } }

  [SerializeField]
  private CanvasGroup _DimBackgroundPrefab;

  private CanvasGroup _DimBackgroundInstance;
  private int _NumDialogsDimmingBackground;
  private Sequence _DimBackgroundTweener;

  [SerializeField]
  private BackgroundColorController _BackgroundColorController;
  public BackgroundColorController BackgroundColorController { get { return _BackgroundColorController; } }

  private List<BaseView> _OpenViews;

  void Awake() {
    Instance = this;
    _OpenViews = new List<BaseView>();
    _NumDialogsDimmingBackground = 0;
    DOTween.Init();
    BaseView.BaseViewCloseAnimationFinished += HandleBaseViewCloseAnimationFinished;
  }

  void Start() {
    BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.Yellow);
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseViews, use OpenView instead.
  /// </summary>
  public static GameObject CreateUIElement(MonoBehaviour uiPrefab) {
    return CreateUIElement(uiPrefab.gameObject, Instance._HorizontalCanvas.transform);
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseViews, use OpenView instead.
  /// </summary>
  public static GameObject CreateUIElement(MonoBehaviour uiPrefab, Transform parentTransform) {
    return CreateUIElement(uiPrefab.gameObject, parentTransform);
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseViews, use OpenView instead.
  /// </summary>
  public static GameObject CreateUIElement(GameObject uiPrefab) {
    GameObject go = null;
    // INGO: Trying fix for https://ankiinc.atlassian.net/browse/COZMO-1172
    if (Instance != null && Instance._HorizontalCanvas != null) {
      go = CreateUIElement(uiPrefab, Instance._HorizontalCanvas.transform);
    }
    return go;
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseViews, use OpenView instead.
  /// </summary>
  public static GameObject CreateUIElement(GameObject uiPrefab, Transform parentTransform) {
    GameObject newUi = Instantiate(uiPrefab);
    newUi.transform.SetParent(parentTransform, false);
    return newUi;
  }

  /// <summary>
  /// Creates a dialog using a script/prefab that extends from BaseView.
  /// Plays open animations on that dialog by default. 
  /// </summary>
  public static T OpenView<T>(
    T viewPrefab,
    System.Action<T> preInitFunc = null,
    bool? overrideBackgroundDim = null,
    bool? overrideCloseOnTouchOutside = null) where T : BaseView {

    GameObject newView = Instantiate(viewPrefab.gameObject);
    T viewScript = newView.GetComponent<T>();

    Transform targetCanvas = Instance._HorizontalCanvas.transform;
    bool shouldDimBackground = overrideBackgroundDim.HasValue ? overrideBackgroundDim.Value : viewScript.DimBackground;
    if (shouldDimBackground) {
      Instance.DimBackground(viewScript, targetCanvas);
    }

    // Set the parent of the dialog after dimmer is created so that it displays
    // on top of the dimmer
    newView.transform.SetParent(targetCanvas, false);

    if (preInitFunc != null) {
      preInitFunc(viewScript);
    }

    viewScript.Initialize(overrideCloseOnTouchOutside);

    SendDasEventForDialogOpen(viewScript);

    Instance._OpenViews.Add(viewScript);

    return viewScript;
  }

  public static void CloseView(BaseView viewObject) {
    viewObject.CloseView();
  }

  public static void CloseViewImmediately(BaseView viewObject) {
    viewObject.CloseViewImmediately();
  }

  public static void CloseAllViews() {
    Instance.CloseAllViewsInternal();
  }

  private void CloseAllViewsInternal() {
    // Close views down the stack
    for (int i = _OpenViews.Count - 1; i >= 0; i--) {
      if (_OpenViews[i] != null) {
        _OpenViews[i].CloseView();
      }
    }
  }

  public static void CloseAllViewsImmediately() {
    if (Instance == null) {
      DAS.Error("UIManager.CloseAllViewsImmediatelyInternal", "Closing views called when UI manager is already destroyed");
      return;
    }
    Instance.CloseAllViewsImmediatelyInternal();
  }

  private void CloseAllViewsImmediatelyInternal() {
    // Close views down the stack
    while (_OpenViews.Count > 0) {
      if (_OpenViews[Instance._OpenViews.Count - 1] != null) {
        DAS.Debug("UIManager.CloseAllViewsImmediatelyInternal", "Closing View " + _OpenViews[Instance._OpenViews.Count - 1].name);
        _OpenViews[Instance._OpenViews.Count - 1].CloseViewImmediately();
      }
    }
  }

  public static Canvas GetUICanvas() {
    return _Instance._HorizontalCanvas;
  }

  public static void DisableTouchEvents() {
    _Instance.EventSystemScript.gameObject.SetActive(false);
  }

  public static void EnableTouchEvents() {
    _Instance.EventSystemScript.gameObject.SetActive(true);
  }

  private void HandleBaseViewCloseAnimationFinished(BaseView view) {
    TryUnDimBackground(view);
    SendDasEventForDialogClose(view);

    _OpenViews.Remove(view);
  }

  private void DimBackground(BaseView view, Transform targetCanvas) {
    view.DimBackground = true;
    _NumDialogsDimmingBackground++;
    if (_NumDialogsDimmingBackground == 1) {
      if (_DimBackgroundInstance == null) {
        // First dialog to dim the background, we need to create a dimmer
        // on top of existing ui
        _DimBackgroundInstance = Instantiate(Instance._DimBackgroundPrefab.gameObject).GetComponent<CanvasGroup>();
        _DimBackgroundInstance.transform.SetParent(targetCanvas, false);
        _DimBackgroundInstance.alpha = 0;
      }

      if (_DimBackgroundTweener != null) {
        _DimBackgroundTweener.Kill();
      }
      _DimBackgroundTweener = DOTween.Sequence();
      _DimBackgroundTweener.Append(_DimBackgroundInstance.DOFade(1, UIDefaultTransitionSettings.Instance.FadeInTransitionDurationSeconds)
                                   .SetEase(UIDefaultTransitionSettings.Instance.FadeInEasing));
    }
  }

  private void TryUnDimBackground(BaseView view) {
    if (view.DimBackground) {
      _NumDialogsDimmingBackground--;
      if (_NumDialogsDimmingBackground <= 0) {
        // Destroy the dimmer when no dialogs want it anymore
        _NumDialogsDimmingBackground = 0;
        if (_DimBackgroundInstance != null) {
          if (_DimBackgroundTweener != null) {
            _DimBackgroundTweener.Kill();
          }
          _DimBackgroundTweener = DOTween.Sequence();
          _DimBackgroundTweener.Append(_DimBackgroundInstance.DOFade(0, UIDefaultTransitionSettings.Instance.FadeOutTransitionDurationSeconds)
                                       .SetEase(UIDefaultTransitionSettings.Instance.FadeOutEasing));
          _DimBackgroundTweener.AppendCallback(() => Destroy(_DimBackgroundInstance.gameObject));
        }
      }
    }
  }

  public void ShowTouchCatcher(System.Action onTouch = null) {
    if (_TouchCatcherInstance == null) {
      _TouchCatcherInstance = Instantiate(_TouchCatcherPrefab).GetComponent<TouchCatcher>();
      _TouchCatcherInstance.transform.SetParent(_Instance._HorizontalCanvas.transform, false);
    }
    _TouchCatcherInstance.Enable();
    if (onTouch != null) {
      _TouchCatcherInstance.OnTouch += onTouch;
    }
  }

  public void HideTouchCatcher() {
    if (_TouchCatcherInstance != null) {
      _TouchCatcherInstance.Disable();
    }
  }

  private static void SendDasEventForDialogOpen(BaseView newView) {
    string eventOpenId = DASUtil.FormatViewTypeForOpen(newView.DASEventViewType);
    SendDialogDasEvent(eventOpenId, newView);
  }

  private static void SendDasEventForDialogClose(BaseView closedView) {
    string eventClosedId = DASUtil.FormatViewTypeForClose(closedView.DASEventViewType);
    SendDialogDasEvent(eventClosedId, closedView);
  }

  private static void SendDialogDasEvent(string eventId, BaseView viewOne) {
    DAS.Event(eventId, viewOne.DASEventViewName);
  }
}
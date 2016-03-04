using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using DG.Tweening;
using Conversations;
using Cozmo.UI;

public class UIManager : MonoBehaviour {

  private static readonly IDAS sDAS = DAS.GetInstance(typeof(UIManager));

  private static UIManager _Instance;

  public static UIManager Instance {
    get {
      if (_Instance == null) {
        sDAS.Error("Don't access this until Start!");
      }
      return _Instance;
    }
    private set {
      if (_Instance != null) {
        sDAS.Error("There shouldn't be more than one UIManager");
      }
      _Instance = value;
    }
  }

  [SerializeField]
  private Canvas _HorizontalCanvas;

  [SerializeField]
  private Canvas _VerticalCanvas;
  
  [SerializeField]
  private EventSystem _EventSystemScript;

  [SerializeField]
  private GameObject _TouchCatcherPrefab;

  [SerializeField]
  private GameObject _DimBackgroundPrefab;

  private List<BaseView> _OpenViews;
  private TouchCatcher _TouchCatcherInstance;
  private GameObject _DimBackgroundInstance;
  private int _NumDialogsDimmingBackground;

  public TouchCatcher TouchCatcher { get { return _TouchCatcherInstance != null && _TouchCatcherInstance.isActiveAndEnabled ? _TouchCatcherInstance : null; } }

  void Awake() {
    Instance = this;
    _OpenViews = new List<BaseView>();
    _NumDialogsDimmingBackground = 0;
    DOTween.Init();
    BaseView.BaseViewCloseAnimationFinished += HandleBaseViewCloseAnimationFinished;
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
    return CreateUIElement(uiPrefab, Instance._HorizontalCanvas.transform);
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseViews, use OpenView instead.
  /// </summary>
  public static GameObject CreateUIElement(GameObject uiPrefab, Transform parentTransform) {
    GameObject newUi = GameObject.Instantiate(uiPrefab);
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
    bool? overrideCloseOnTouchOutside = null, 
    bool verticalCanvas = false) where T : BaseView {

    GameObject newView = GameObject.Instantiate(viewPrefab.gameObject);
    T viewScript = newView.GetComponent<T>();

    Transform targetCanvas = verticalCanvas ? Instance._VerticalCanvas.transform : Instance._HorizontalCanvas.transform;
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
    Instance.CloseAllViewsImmediatelyInternal();
  }

  private void CloseAllViewsImmediatelyInternal() {
    // Close views down the stack
    while (_OpenViews.Count > 0) {
      if (_OpenViews[Instance._OpenViews.Count - 1] != null) {
        _OpenViews[Instance._OpenViews.Count - 1].CloseViewImmediately();
      }
    }
  }

  public static Canvas GetUICanvas() {
    return _Instance._HorizontalCanvas;
  }

  public static void DisableTouchEvents() {
    _Instance._EventSystemScript.gameObject.SetActive(false);
  }

  public static void EnableTouchEvents() {
    _Instance._EventSystemScript.gameObject.SetActive(true);
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
      // First dialog to dim the background, we need to create a dimmer
      // on top of existing ui
      _DimBackgroundInstance = GameObject.Instantiate(Instance._DimBackgroundPrefab);
      _DimBackgroundInstance.transform.SetParent(targetCanvas, false);
    }
  }

  private void TryUnDimBackground(BaseView view) {
    if (view.DimBackground) {
      _NumDialogsDimmingBackground--;
      if (_NumDialogsDimmingBackground <= 0) {
        // Destroy the dimmer when no dialogs want it anymore
        _NumDialogsDimmingBackground = 0;
        if (_DimBackgroundInstance != null) {
          Destroy(_DimBackgroundInstance);
        }
      }
    }
  }

  public void ShowTouchCatcher(System.Action onTouch = null) {
    if (_TouchCatcherInstance == null) {
      _TouchCatcherInstance = GameObject.Instantiate<GameObject>(_TouchCatcherPrefab).GetComponent<TouchCatcher>();
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
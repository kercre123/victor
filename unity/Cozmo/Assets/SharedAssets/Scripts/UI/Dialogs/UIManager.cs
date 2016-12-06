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
  private int _NumModalsDimmingBackground;
  private Sequence _DimBackgroundTweener;

  [SerializeField]
  private BackgroundColorController _BackgroundColorController;
  public BackgroundColorController BackgroundColorController { get { return _BackgroundColorController; } }

  private List<BaseModal> _OpenModals;
  // Account far the fact that there's always a base view open
  public int NumberOfOpenDialogues() { return _OpenModals.Count - 1; }

  void Awake() {
    Instance = this;
    _OpenModals = new List<BaseModal>();
    _NumModalsDimmingBackground = 0;
    DOTween.Init();
    BaseModal.BaseViewCloseAnimationFinished += HandleBaseViewCloseAnimationFinished;
  }

  void Start() {
    BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.Yellow);
  }

  private void OnDestroy() {
    if (_DimBackgroundTweener != null) {
      _DimBackgroundTweener.Kill();
    }
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseModals, use OpenModal<T> instead.
  /// </summary>
  public static GameObject CreateUIElement(MonoBehaviour uiPrefab) {
    return CreateUIElement(uiPrefab.gameObject, Instance._HorizontalCanvas.transform);
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseModals, use OpenModal<T> instead.
  /// </summary>
  public static GameObject CreateUIElement(MonoBehaviour uiPrefab, Transform parentTransform) {
    return CreateUIElement(uiPrefab.gameObject, parentTransform);
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseModals, use OpenModal<T> instead.
  /// </summary>
  public static GameObject CreateUIElement(GameObject uiPrefab) {
    GameObject go = null;
    // INGO: Trying fix for https://ankiinc.atlassian.net/browse/COZMO-1172
    if (_Instance != null && _Instance._HorizontalCanvas != null) {
      go = CreateUIElement(uiPrefab, _Instance._HorizontalCanvas.transform);
    }
    return go;
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseModals, use OpenModal<T> instead.
  /// </summary>
  public static GameObject CreateUIElement(GameObject uiPrefab, Transform parentTransform) {
    GameObject newUi = Instantiate(uiPrefab);
    newUi.transform.SetParent(parentTransform, false);
    return newUi;
  }

  /// <summary>
  /// Creates a dialog using a script/prefab that extends from BaseModal.
  /// Plays open animations on that dialog by default. 
  /// </summary>
  public static T OpenModal<T>(
    T modalPrefab,
    System.Action<T> creationSuccessCallback = null,
    bool? overrideBackgroundDim = null,
    bool? overrideCloseOnTouchOutside = null) where T : BaseModal {

    GameObject newModal = Instantiate(modalPrefab.gameObject);
    T modalScript = newModal.GetComponent<T>();

    Transform targetCanvas = _Instance._HorizontalCanvas.transform;
    newModal.transform.SetParent(targetCanvas, false);

    if (creationSuccessCallback != null) {
      creationSuccessCallback(modalScript);
    }

    modalScript.Initialize(overrideCloseOnTouchOutside);

    SendDasEventForDialogOpen(modalScript);

    _Instance._OpenModals.Add(modalScript);

    // find the spot in the transform hierarchy that viewScript belongs in based on layer priority
    for (int i = 0; i < _Instance._OpenModals.Count; ++i) {
      if (_Instance._OpenModals[i].LayerPriority > modalScript.LayerPriority) {
        modalScript.transform.SetSiblingIndex(_Instance._OpenModals[i].transform.GetSiblingIndex());
        break;
      }
    }

    bool shouldDimBackground = overrideBackgroundDim.HasValue ? overrideBackgroundDim.Value : modalScript.DimBackground;
    if (shouldDimBackground) {
      _Instance.DimBackground(modalScript, targetCanvas);
    }

    _Instance.PlaceDimmer();

    return modalScript;
  }

  public static void CloseModal(BaseModal modalObject) {
    modalObject.CloseView();
  }

  public static void CloseModalImmediately(BaseModal modalObject) {
    modalObject.CloseViewImmediately();
  }

  public static void CloseAllModals() {
    _Instance.CloseAllModalsInternal();
  }

  private void CloseAllModalsInternal() {
    // Close views down the stack
    for (int i = _OpenModals.Count - 1; i >= 0; i--) {
      if (_OpenModals[i] != null) {
        _OpenModals[i].CloseView();
      }
    }
  }

  public static void CloseAllModalsImmediately() {
    if (_Instance == null) {
      DAS.Error("UIManager.CloseAllModalsImmediately", "Closing modals called when UI manager is already destroyed");
      return;
    }
    _Instance.CloseAllModalsImmediatelyInternal();
  }

  private void CloseAllModalsImmediatelyInternal() {
    // Close views down the stack
    while (_OpenModals.Count > 0) {
      if (_OpenModals[_OpenModals.Count - 1] != null) {
        DAS.Debug("UIManager.CloseAllModalsImmediatelyInternal", "Closing Modals " + _OpenModals[Instance._OpenModals.Count - 1].name);
        _OpenModals[_OpenModals.Count - 1].CloseViewImmediately();
      }
    }
  }

  public static Canvas GetUICanvas() {
    return _Instance._HorizontalCanvas;
  }

  public static void DisableTouchEvents() {
    if (_Instance != null && _Instance.EventSystemScript != null) {
      _Instance.EventSystemScript.gameObject.SetActive(false);
    }
  }

  public static void EnableTouchEvents() {
    if (_Instance != null && _Instance.EventSystemScript != null) {
      _Instance.EventSystemScript.gameObject.SetActive(true);
    }
  }

  public static string GetTopModalName() {
    var openModals = _Instance._OpenModals;
    var openModal = openModals.Count > 0 ? openModals[openModals.Count - 1] : null;
    return openModal != null ? openModal.name : string.Empty;
  }

  private void HandleBaseViewCloseAnimationFinished(BaseModal view) {
    _OpenModals.Remove(view);
    TryUnDimBackground(view);
    SendDasEventForDialogClose(view);
  }

  private void DimBackground(BaseModal modalObject, Transform targetCanvas) {
    modalObject.DimBackground = true;
    _NumModalsDimmingBackground++;
    if (_NumModalsDimmingBackground == 1) {
      if (_DimBackgroundInstance == null) {
        // First dialog to dim the background, we need to create a dimmer
        // on top of existing ui, if the view has a specific prefab it wants
        // to use for dimming the background, use that instead
        // (For instance, onboarding background dimming that wants to avoid
        // covering the entire screen)
        GameObject toInstantiate = _DimBackgroundPrefab.gameObject;
        _DimBackgroundInstance = Instantiate(toInstantiate).GetComponent<CanvasGroup>();
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

  private void TryUnDimBackground(BaseModal modalObject) {
    if (modalObject.DimBackground) {
      _NumModalsDimmingBackground--;
      if (_NumModalsDimmingBackground <= 0) {
        // Destroy the dimmer when no dialogs want it anymore
        _NumModalsDimmingBackground = 0;
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
    PlaceDimmer();
  }

  private void PlaceDimmer() {
    // find where transform hierarchy the dim background should go
    if (_DimBackgroundInstance != null) {
      for (int i = _OpenModals.Count - 1; i >= 0; --i) {
        if (_OpenModals[i].DimBackground) {
          int targetIndex = _OpenModals[i].transform.GetSiblingIndex();
          int currentDimmerIndex = _DimBackgroundInstance.transform.GetSiblingIndex();
          if (currentDimmerIndex < targetIndex) {
            _DimBackgroundInstance.transform.SetSiblingIndex(targetIndex - 1);
          }
          else {
            _DimBackgroundInstance.transform.SetSiblingIndex(targetIndex);
          }
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

  private static void SendDasEventForDialogOpen(BaseModal newView) {
    string eventOpenId = DASUtil.FormatViewTypeForOpen(newView.DASEventViewType);
    SendDialogDasEvent(eventOpenId, newView);
  }

  private static void SendDasEventForDialogClose(BaseModal closedView) {
    string eventClosedId = DASUtil.FormatViewTypeForClose(closedView.DASEventViewType);
    SendDialogDasEvent(eventClosedId, closedView);
  }

  private static void SendDialogDasEvent(string eventId, BaseModal viewOne) {
    DAS.Event(eventId, viewOne.DASEventViewName);
  }
}
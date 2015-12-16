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
  private Canvas _OverlayCanvas;
  
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

  public static GameObject CreateUIElement(MonoBehaviour uiPrefab) {
    return CreateUIElement(uiPrefab.gameObject, Instance._OverlayCanvas.transform);
  }

  public static GameObject CreateUIElement(MonoBehaviour uiPrefab, Transform parentTransform) {
    return CreateUIElement(uiPrefab.gameObject, parentTransform);
  }

  public static GameObject CreateUIElement(GameObject uiPrefab) {
    return CreateUIElement(uiPrefab, Instance._OverlayCanvas.transform);
  }

  public static GameObject CreateUIElement(GameObject uiPrefab, Transform parentTransform) {
    GameObject newUi = GameObject.Instantiate(uiPrefab);
    newUi.transform.SetParent(parentTransform, false);
    return newUi;
  }

  public static BaseView OpenView(BaseView viewPrefab, bool animateImmediately = true) {
    GameObject newView = GameObject.Instantiate(viewPrefab.gameObject);

    BaseView baseViewScript = newView.GetComponent<BaseView>();
    Instance._OpenViews.Add(baseViewScript);

    if (baseViewScript.DimBackground) {
      Instance._NumDialogsDimmingBackground++;
      if (Instance._NumDialogsDimmingBackground == 1) {
        // First dialog to dim the background, we need to create a dimmer
        // on top of existing ui
        Instance._DimBackgroundInstance = GameObject.Instantiate(Instance._DimBackgroundPrefab);
        Instance._DimBackgroundInstance.transform.SetParent(Instance._OverlayCanvas.transform, false);
      }
    }

    // Set the parent of the dialog after dimmer is created so that it displays
    // on top of the dimmer
    newView.transform.SetParent(Instance._OverlayCanvas.transform, false);

    if (animateImmediately) {
      baseViewScript.OpenView();
    }

    return baseViewScript;
  }

  public static void CloseView(BaseView viewObject) {
    viewObject.CloseView();
  }

  public static void CloseViewImmediately(BaseView viewObject) {
    viewObject.CloseViewImmediately();
  }

  public static void CloseAllViews() {
    while (Instance._OpenViews.Count > 0) {
      if (Instance._OpenViews[0] != null) {
        Instance._OpenViews[0].CloseView();
      }
    }
  }

  public static void CloseAllViewsImmediately() {
    while (Instance._OpenViews.Count > 0) {
      if (Instance._OpenViews[0] != null) {
        Instance._OpenViews[0].CloseViewImmediately();
      }
    }
  }

  public static Camera GetUICamera() {
    return _Instance._OverlayCanvas.worldCamera;
  }

  public static void DisableTouchEvents() {
    _Instance._EventSystemScript.gameObject.SetActive(false);
  }

  public static void EnableTouchEvents() {
    _Instance._EventSystemScript.gameObject.SetActive(true);
  }

  private void HandleBaseViewCloseAnimationFinished(BaseView view) {
    if (view.DimBackground) {
      Instance._NumDialogsDimmingBackground--;
      if (Instance._NumDialogsDimmingBackground <= 0) {
        // Destroy the dimmer when no dialogs want it anymore
        Instance._NumDialogsDimmingBackground = 0;
        if (Instance._DimBackgroundInstance != null) {
          Destroy(Instance._DimBackgroundInstance);
        }
      }
    }

    Instance._OpenViews.Remove(view);
  }

  public void ShowTouchCatcher(System.Action onTouch = null) {
    if (_TouchCatcherInstance == null) {
      _TouchCatcherInstance = GameObject.Instantiate<GameObject>(_TouchCatcherPrefab).GetComponent<TouchCatcher>();
      _TouchCatcherInstance.transform.SetParent(_Instance._OverlayCanvas.transform, false);
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
}
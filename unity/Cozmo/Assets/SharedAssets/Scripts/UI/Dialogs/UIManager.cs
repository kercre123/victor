using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using DG.Tweening;
using Conversations;

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

  private List<BaseView> _OpenViews;

  private TouchCatcher _TouchCatcherInstance;

  public TouchCatcher TouchCatcher { get { return _TouchCatcherInstance != null && _TouchCatcherInstance.isActiveAndEnabled ? _TouchCatcherInstance : null; } }

  void Awake() {
    Instance = this;
    _OpenViews = new List<BaseView>();
    DOTween.Init();
    BaseView.BaseViewCloseAnimationFinished += HandleBaseViewCloseAnimationFinished;
  }

  public static GameObject CreateUIElement(GameObject uiPrefab) {
    return CreateUIElement(uiPrefab, Instance._OverlayCanvas.transform);
  }

  public static GameObject CreateUIElement(GameObject uiPrefab, Transform parentTransform) {
    GameObject newUi = GameObject.Instantiate(uiPrefab);
    newUi.transform.SetParent(parentTransform, false);
    return newUi;
  }

  public static BaseView OpenView(BaseView viewPrefab) {
    GameObject newView = GameObject.Instantiate(viewPrefab.gameObject);
    newView.transform.SetParent(Instance._OverlayCanvas.transform, false);

    BaseView baseViewScript = newView.GetComponent<BaseView>();
    baseViewScript.OpenView();
    Instance._OpenViews.Add(baseViewScript);

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

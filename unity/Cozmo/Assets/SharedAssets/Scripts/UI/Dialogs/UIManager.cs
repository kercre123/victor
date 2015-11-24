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
  private Canvas _overlayCanvas;
  
  [SerializeField]
  private EventSystem _eventSystemScript;

  private List<BaseView> _openViews;

  void Awake() {
    _Instance = this;
    _openViews = new List<BaseView>();
    DOTween.Init();
    BaseView.BaseViewCloseAnimationFinished += HandleBaseViewCloseAnimationFinished;
  }

  public static GameObject CreateUIElement(GameObject uiPrefab) {
    return CreateUIElement(uiPrefab, Instance._overlayCanvas.transform);
  }

  public static GameObject CreateUIElement(GameObject uiPrefab, Transform parentTransform) {
    GameObject newUi = GameObject.Instantiate(uiPrefab);
    newUi.transform.SetParent(parentTransform, false);
    return newUi;
  }

  public static BaseView OpenView(BaseView viewPrefab) {
    GameObject newView = GameObject.Instantiate(viewPrefab.gameObject);
    newView.transform.SetParent(Instance._overlayCanvas.transform, false);

    BaseView baseViewScript = newView.GetComponent<BaseView>();
    baseViewScript.OpenView();
    Instance._openViews.Add(baseViewScript);

    return baseViewScript;
  }

  public static void CloseView(BaseView viewObject) {
    viewObject.CloseView();
  }

  public static void CloseViewImmediately(BaseView viewObject) {
    viewObject.CloseViewImmediately();
  }

  public static void CloseAllViews() {
    while (Instance._openViews.Count > 0) {
      if (Instance._openViews[0] != null) {
        Instance._openViews[0].CloseView();
      }
    }
  }

  public static void CloseAllViewsImmediately() {
    while (Instance._openViews.Count > 0) {
      if (Instance._openViews[0] != null) {
        Instance._openViews[0].CloseViewImmediately();
      }
    }
  }

  public static Camera GetUICamera() {
    return _Instance._overlayCanvas.worldCamera;
  }

  public static void DisableTouchEvents() {
    _Instance._eventSystemScript.gameObject.SetActive(false);
  }

  public static void EnableTouchEvents() {
    _Instance._eventSystemScript.gameObject.SetActive(true);
  }

  private void HandleBaseViewCloseAnimationFinished(BaseView view) {
    Instance._openViews.Remove(view);
  }

}

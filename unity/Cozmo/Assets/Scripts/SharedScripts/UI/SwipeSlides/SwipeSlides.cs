using UnityEngine;
using UnityEngine.EventSystems;
using System.Collections.Generic;
using DG.Tweening;
using Cozmo.UI;

public struct TouchInfo {
  public TouchInfo(Vector2 position, float time) {
    _Position = position;
    _Time = time;
  }
  public Vector2 _Position;
  public float _Time;
}

public class SwipeSlides : MonoBehaviour {

  [SerializeField]
  private GameObject[] _SlidePrefabs;
  private List<GameObject> _SlideInstances = new List<GameObject>();

  [SerializeField]
  private RectTransform _SwipeContainer;

  [SerializeField]
  private SwipePageIndicator _PageIndicatorPrefab;
  private SwipePageIndicator _PageIndicatorInstance;

  //private float _ThresholdSpeed;
  //private Dictionary<int, TouchInfo> _StartTouchInfo = new Dictionary<int, TouchInfo>();

  private int _CurrentIndex = 0;
  private bool _Transitioning = false;
  private Vector3 _StartingPosition;

  private GameObject _SwipeContainerMask;
  private Tweener _SwipeTween;

  private RectTransform _RectTransform;

  private void Start() {
    if (_SlidePrefabs.Length > 0) {
      Initialize(_SlidePrefabs);
    }
  }

  public void Initialize(GameObject[] slidePrefabs) {
    _RectTransform = GetComponent<RectTransform>();

    for (int i = 0; i < slidePrefabs.Length; ++i) {
      GameObject slideInstance = GameObject.Instantiate(slidePrefabs[i]);
      slideInstance.transform.SetParent(_SwipeContainer, false);
      slideInstance.transform.localPosition = new Vector3(i * GetComponent<RectTransform>().rect.width, slideInstance.transform.localPosition.y, slideInstance.transform.localPosition.z);
      _SlideInstances.Add(slideInstance);
    }
    _StartingPosition = _SwipeContainer.localPosition;

    _SwipeContainerMask = _SwipeContainer.transform.parent.gameObject;

    _PageIndicatorInstance = GameObject.Instantiate(_PageIndicatorPrefab.gameObject).GetComponent<SwipePageIndicator>();
    _PageIndicatorInstance.transform.SetParent(_SwipeContainerMask.transform, false);
    _PageIndicatorInstance.SetPageCount(slidePrefabs.Length);
    _PageIndicatorInstance.SetCurrentPage(_CurrentIndex);
    _PageIndicatorInstance.OnNextButton += TransitionRight;
    _PageIndicatorInstance.OnBackButton += TransitionLeft;

    // Makes sure everything is not active and hidden.
    // huge perf boost on low end phones because it still counts as a triangle even when masked
    TransitionDone();
  }

  public GameObject GetSlideInstanceAt(int index) {
    return _SlideInstances[index];
  }

  public void GoToIndex(int index) {
    if (index < 0 || index >= _SlideInstances.Count) {
      DAS.Error("SwipeSlides.GoToIndex", "out of bounds: " + index);
      return;
    }
    if (_Transitioning) {
      return;
    }
    if (_SwipeTween != null) {
      _SwipeTween.Kill();
      _SwipeTween = null;
    }
    _Transitioning = true;

    _SlideInstances[index].SetActive(true);

    Vector3 unitsToMove = _StartingPosition - Vector3.right * _RectTransform.rect.width * index;
    _SwipeTween = _SwipeContainer.DOLocalMove(unitsToMove,
                                              UIDefaultTransitionSettings.Instance.SwipeDurationSeconds)
                                 .SetEase(UIDefaultTransitionSettings.Instance.SwipeSlideEase)
                                 .OnComplete(() => TransitionDone());
  }

  private void TransitionDone() {
    _Transitioning = false;
    _PageIndicatorInstance.SetCurrentPage(_CurrentIndex);

    for (int i = 0; i < _SlideInstances.Count; ++i) {
      _SlideInstances[i].SetActive(i == _CurrentIndex);
    }
  }
  private void TransitionLeft() {
    if (_CurrentIndex <= 0 || _Transitioning) {
      return;
    }
    _CurrentIndex--;
    GoToIndex(_CurrentIndex);
  }

  private void TransitionRight() {
    if (_CurrentIndex >= _SlideInstances.Count - 1 || _Transitioning) {
      return;
    }
    _CurrentIndex++;
    GoToIndex(_CurrentIndex);
  }

  private void OnDestroy() {
    for (int i = 0; i < _SlideInstances.Count; ++i) {
      GameObject.Destroy(_SlideInstances[i]);
    }
    _SlideInstances.Clear();
    if (_SwipeTween != null) {
      _SwipeTween.Kill();
      _SwipeTween = null;
    }
  }

}

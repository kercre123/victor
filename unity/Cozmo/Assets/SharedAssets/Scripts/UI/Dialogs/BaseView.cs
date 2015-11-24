using UnityEngine;
using System.Collections;
using DG.Tweening;

public abstract class BaseView : MonoBehaviour {
  
  public delegate void SimpleBaseViewHandler();

  public delegate void BaseViewHandler(BaseView view);

  public event SimpleBaseViewHandler ViewOpened;
  public static event BaseViewHandler BaseViewOpened;

  private static void RaiseViewOpened(BaseView view) {
    if (view.ViewOpened != null) {
      view.ViewOpened();
    }
    if (BaseViewOpened != null) {
      BaseViewOpened(view);
    }
  }

  public event SimpleBaseViewHandler ViewOpenAnimationFinished;
  public static event BaseViewHandler BaseViewOpenAnimationFinished;

  private static void RaiseViewOpenAnimationFinished(BaseView view) {
    if (view.ViewOpenAnimationFinished != null) {
      view.ViewOpenAnimationFinished();
    }
    if (BaseViewOpenAnimationFinished != null) {
      BaseViewOpenAnimationFinished(view);
    }
  }

  public event SimpleBaseViewHandler ViewClosed;
  public static event BaseViewHandler BaseViewClosed;

  private static void RaiseViewClosed(BaseView view) {
    if (view.ViewClosed != null) {
      view.ViewClosed();
    }
    if (BaseViewClosed != null) {
      BaseViewClosed(view);
    }
  }

  public event SimpleBaseViewHandler ViewCloseAnimationFinished;
  public static event BaseViewHandler BaseViewCloseAnimationFinished;

  private static void RaiseViewCloseAnimationFinished(BaseView view) {
    if (view.ViewCloseAnimationFinished != null) {
      view.ViewCloseAnimationFinished();
    }
    if (BaseViewCloseAnimationFinished != null) {
      BaseViewCloseAnimationFinished(view);
    }
  }

  private Sequence _transitionAnimation;

  public void OnDestroy() {
    if (_transitionAnimation != null) {
      _transitionAnimation.Kill();
    }

    CleanUp();
  }

  protected abstract void CleanUp();

  public void OpenView() {
    RaiseViewOpened(this);
    PlayOpenAnimations();
  }

  public void CloseView() {
    RaiseViewClosed(this);
    PlayCloseAnimations();
  }

  public void CloseViewImmediately() {
    RaiseViewClosed(this);

    // Close dialog without playing animations
    OnCloseAnimationsFinished();
  }

  private void PlayOpenAnimations() {
    UIManager.DisableTouchEvents();

    // Play some animations
    if (_transitionAnimation != null) {
      _transitionAnimation.Kill();
    }
    _transitionAnimation = DOTween.Sequence();
    ConstructOpenAnimation(_transitionAnimation);
    _transitionAnimation.AppendCallback(OnOpenAnimationsFinished);
  }

  protected virtual void ConstructOpenAnimation(Sequence openAnimation) {
  }

  private void OnOpenAnimationsFinished() {
    UIManager.EnableTouchEvents();

    // Raise event
    RaiseViewOpenAnimationFinished(this);
  }

  private void PlayCloseAnimations() {
    UIManager.DisableTouchEvents();

    // Play some animations
    if (_transitionAnimation != null) {
      _transitionAnimation.Kill();
    }
    _transitionAnimation = DOTween.Sequence();
    ConstructCloseAnimation(_transitionAnimation);
    _transitionAnimation.AppendCallback(OnCloseAnimationsFinished);

  }

  // TODO: Make virtual function play from a set of default animations based on a serialized enum
  // plus the pivot point and direction (top, bottom, left, right)
  // TODO: Make protected functions that return default tweeners you can add to the sequence
  protected virtual void ConstructCloseAnimation(Sequence closeAnimation) {
  }

  private void OnCloseAnimationsFinished() {
    UIManager.EnableTouchEvents();
    RaiseViewCloseAnimationFinished(this);

    GameObject.Destroy(gameObject);
  }
}

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

  private Sequence _closeAnimation;

  public void OnDestroy() {
    if (_closeAnimation != null) {
      _closeAnimation.Kill();
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

    // TODO: Play some animations

    OnOpenAnimationsFinished();
  }

  private void OnOpenAnimationsFinished() {
    UIManager.EnableTouchEvents();

    // TODO: Raise event
  }

  private void PlayCloseAnimations() {
    UIManager.DisableTouchEvents();

    // Play some animations
    _closeAnimation = DOTween.Sequence();
    ConstructCloseAnimation(_closeAnimation);
    _closeAnimation.AppendCallback(OnCloseAnimationsFinished);

  }

  protected abstract void ConstructCloseAnimation(Sequence closeAnimation);

  private void OnCloseAnimationsFinished() {
    UIManager.EnableTouchEvents();
    RaiseViewCloseAnimationFinished(this);

    GameObject.Destroy(gameObject);
  }
}

using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class BaseView : BaseDialog {

      public delegate void BaseViewHandler(BaseView view);

      public static event BaseViewHandler BaseViewOpened;
      public static event BaseViewHandler BaseViewOpenAnimationFinished;
      public static event BaseViewHandler BaseViewClosed;
      public static event BaseViewHandler BaseViewCloseAnimationFinished;

      protected override void RaiseDialogOpened() {
        base.RaiseDialogOpened();
        if (BaseViewOpened != null) {
          BaseViewOpened(this);
        }
      }

      protected override void RaiseDialogOpenAnimationFinished() {
        base.RaiseDialogOpenAnimationFinished();
        if (BaseViewOpenAnimationFinished != null) {
          BaseViewOpenAnimationFinished(this);
        }
      }

      protected override void RaiseDialogClosed() {
        base.RaiseDialogClosed();
        if (BaseViewClosed != null) {
          BaseViewClosed(this);
        }
      }

      protected override void RaiseDialogCloseAnimationFinished() {
        base.RaiseDialogCloseAnimationFinished();
        if (BaseViewCloseAnimationFinished != null) {
          BaseViewCloseAnimationFinished(this);
        }
      }
    }
  }
}
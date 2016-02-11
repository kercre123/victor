using DG.Tweening;
using UnityEngine;

namespace Cozmo {
  namespace MinigameWidgets {
    // Upgrading to a abstract class so that BaseViews can't interface them
    // They're not really an interface anyway.
    public abstract class MinigameWidget : MonoBehaviour {

      public virtual void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      public abstract Sequence OpenAnimationSequence();

      public abstract Sequence CloseAnimationSequence();
    }
  }
}
using DG.Tweening;
using UnityEngine;

namespace Cozmo {
  namespace MinigameWidgets {
    // Upgrading to a abstract class so that BaseViews can't interface them
    // They're not really an interface anyway.
    public abstract class MinigameWidget : MonoBehaviour {
      
      public virtual void DestroyWidgetImmediately() {
        if (gameObject != null) {
          DAS.Info(this, "Destroying Widget: " + gameObject.name);
          Destroy(gameObject);
        }
      }

      private void OnDestroy() {
        DAS.Info(this, "MinigameWidget OnDestroy " + gameObject.name);
        if (OpenSequence != null) {
          OpenSequence.Kill();
        }
        if (CloseSequence != null) {
          CloseSequence.Kill();
        }
      }

      /// <summary>
      /// Create an Open Animation Sequence for this widget based on its default parameters.
      /// </summary>
      /// <returns>The open animation sequence.</returns>
      public abstract Sequence CreateOpenAnimSequence();

      /// <summary>
      /// Creates the open animation sequence. 
      /// Offsets are relative to the Widgets actual position in the UI after the sequence ends.
      /// </summary>
      /// <returns>The open animation sequence.</returns>
      /// <param name="xOffset">X offset for the position the widget is coming from.</param>
      /// <param name="yOffset">Y offset for the position the widget is coming from.</param>
      /// <param name="duration">Duration for the tween sequence.</param>
      public virtual Sequence CreateOpenAnimSequence(float xOffset, float yOffset, float duration) {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + xOffset, 
          this.transform.localPosition.y + yOffset, this.transform.localPosition.z),
          duration).From().SetEase(Ease.OutBack));
        if (OpenSequence != null) {
          OpenSequence.Kill();
        }
        OpenSequence = open;
        return open;
      }

      /// <summary>
      /// Create a Close Animation Sequence for this widget based on its default parameters.
      /// </summary>
      /// <returns>The close animation sequence.</returns>
      public abstract Sequence CreateCloseAnimSequence();

      /// <summary>
      /// Creates the close animation sequence.
      /// Offsets are relative to the Widgets actual position in the UI before the sequence starts.
      /// </summary>
      /// <returns>The close animation sequence.</returns>
      /// <param name="xOffset">X offset for the position the widget is going to.</param>
      /// <param name="yOffset">Y offset for the position the widget is going to.</param>
      /// <param name="duration">Duration for the tween sequence.</param>
      public virtual Sequence CreateCloseAnimSequence(float xOffset, float yOffset, float duration) {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + xOffset, 
          this.transform.localPosition.y + yOffset, this.transform.localPosition.z),
          duration).SetEase(Ease.OutQuad));
        if (CloseSequence != null) {
          CloseSequence.Kill();
        }
        CloseSequence = close;
        return close;
      }

      // Reference to the Open Sequence of this Widget
      public Sequence OpenSequence;
      // Reference to the Close Sequence of this Widget
      public Sequence CloseSequence;
    }
  }
}
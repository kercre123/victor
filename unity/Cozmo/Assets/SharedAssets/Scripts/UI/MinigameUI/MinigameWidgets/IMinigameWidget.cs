using DG.Tweening;

namespace Cozmo {
  namespace MinigameWidgets {
    public interface IMinigameWidget {

      void DestroyWidgetImmediately();

      Sequence OpenAnimationSequence();

      Sequence CloseAnimationSequence();

      void EnableInteractivity();

      void DisableInteractivity();
    }
  }
}
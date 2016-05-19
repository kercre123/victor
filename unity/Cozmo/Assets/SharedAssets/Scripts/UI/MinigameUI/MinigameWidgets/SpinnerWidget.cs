using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class SpinnerWidget : MinigameWidget {

      private const float kAnimXOffset = 0.0f;
      private const float kAnimYOffset = -300.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private RectTransform _SpinnerTransform;

      [SerializeField]
      private GameObject _SpinnerPrefab;
      private GameObject _SpinnerInstance;

      private void Awake() {
        _SpinnerInstance = GameObject.Instantiate(_SpinnerPrefab);
        _SpinnerInstance.transform.SetParent(_SpinnerTransform, false);
      }

      public override void DestroyWidgetImmediately() {
        Destroy(_SpinnerInstance);
        Destroy(gameObject);
      }

      public override Sequence CreateOpenAnimSequence() {
        return CreateOpenAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      public override Sequence CreateCloseAnimSequence() {
        return CreateCloseAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }
    }
  }
}

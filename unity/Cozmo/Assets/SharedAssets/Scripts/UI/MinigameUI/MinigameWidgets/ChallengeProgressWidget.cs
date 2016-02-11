using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.UI;
using DG.Tweening;
using System.Collections.Generic;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ChallengeProgressWidget : MinigameWidget {
      [SerializeField]
      private ProgressBar _ChallengeProgressBar;

      [SerializeField]
      private AnkiTextLabel _ProgressBarLabel;

      [SerializeField]
      private HorizontalOrVerticalLayoutGroup _ProgressBarSegmentsContainer;

      [SerializeField]
      private GameObject _SegmentMarkerPrefab;

      private List<GameObject> _SegmentMarkerObjects;

      private float _SegmentWidthPixels = -1;

      #region IMinigameWidget

      public override void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public override Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 600, 
          this.transform.localPosition.y - 300, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public override Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 600, 
          this.transform.localPosition.y - 300, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      #endregion

      public string ProgressBarLabelText {
        get {
          return _ProgressBarLabel.text;
        }
        set {
          _ProgressBarLabel.text = value;
        }
      }

      public void ResetProgress() {
        _ChallengeProgressBar.ResetProgress();
      }

      public void SetProgress(float progress) {
        _ChallengeProgressBar.SetProgress(progress);
      }

      public int NumSegments {
        get {
          return _SegmentMarkerObjects != null ? _SegmentMarkerObjects.Count + 1 : 1;
        }
        set {
          if (_SegmentMarkerObjects == null) {
            _SegmentMarkerObjects = new List<GameObject>();
          }

          if (value > _SegmentMarkerObjects.Count + 1) {
            GameObject newMarker;
            LayoutElement layoutElement;
            while (value > _SegmentMarkerObjects.Count + 1) {
              newMarker = UIManager.CreateUIElement(_SegmentMarkerPrefab, _ProgressBarSegmentsContainer.transform);
              if (_SegmentWidthPixels == -1) {
                layoutElement = newMarker.GetComponent<LayoutElement>();
                _SegmentWidthPixels = layoutElement != null ? layoutElement.preferredWidth : 0;
              }
              _SegmentMarkerObjects.Add(newMarker);
            }

            float containerWidth = (_ProgressBarSegmentsContainer.transform as RectTransform).rect.width;
            float spacing = (containerWidth - (value - 1) * _SegmentWidthPixels) / value;
            _ProgressBarSegmentsContainer.spacing = spacing;
          }
          else if (value < _SegmentMarkerObjects.Count + 1) {
            // Clamp number of segments to 1
            int newNumSegments = value > 1 ? value : 1;
            GameObject removeAt;
            while (newNumSegments < _SegmentMarkerObjects.Count + 1) {
              removeAt = _SegmentMarkerObjects[0];
              _SegmentMarkerObjects.RemoveAt(0);
              Destroy(removeAt);
            }
          }
        }
      }
    }
  }
}
using UnityEngine;
using UnityEngine.UI;
using System.Collections.Generic;

namespace Cozmo {
  namespace UI {
    // This potentially has a lot of overlap with ProgressBarWidget,
    // especially if we make use of masks to achieve the same effect.
    // This also doesn't have tweening, each segment is strictly on/off.
    public class SegmentedBar : MonoBehaviour {

      [SerializeField]
      private Toggle _SegmentPrefab;

      [SerializeField]
      private HorizontalOrVerticalLayoutGroup _SegmentContainer;

      [SerializeField]
      private string _OnImageSkinComponentId;

      [SerializeField]
      private string _OffImageSkinComponentId;

      // Use Unity.UI.Toggles for now as the "segments"
      protected List<Toggle> _CurrentSegments = new List<Toggle>();

      public void SetMaximumSegments(int maxNumSegments) {
        if (maxNumSegments < 0) {
          DAS.Warn("SegmentedBar.SetMaximumSegments.NegativeSegments", "Trying to set a negative number of segments! Clamping to 0.");
          maxNumSegments = 0;
        }

        //Make sure _CurrentSegments is up to date, object may have been refreshed
        if (_CurrentSegments.Count == 0 && _SegmentContainer.transform.childCount != _CurrentSegments.Count) {
          for (int i = 0; i < _SegmentContainer.transform.childCount; i++) {
            Toggle toggleItem = _SegmentContainer.transform.GetChild(i).GetComponent<Toggle>();
            if (toggleItem != null) {
              _CurrentSegments.Add(toggleItem);
            }
          }
        }

        // Grow the bar if the desired maximum is more than the current max
        if (_CurrentSegments.Count < maxNumSegments) {
          for (int i = _CurrentSegments.Count; i < maxNumSegments; i++) {
            _CurrentSegments.Add(CreateSegment());
          }
        }
        // Shrink the bar if the desired maximum is less than the current max
        else if (_CurrentSegments.Count > maxNumSegments) {
          Toggle toRemove;
          while (maxNumSegments < _CurrentSegments.Count) {
            // Remove segments from the end of the list
            toRemove = _CurrentSegments[_CurrentSegments.Count - 1];
            _CurrentSegments.RemoveAt(_CurrentSegments.Count - 1);
            DeleteSegment(toRemove);
          }
        }
      }

      public void SetCurrentNumSegments(int currentNumSegments) {
        if (currentNumSegments < 0) {
          DAS.Warn("SegmentedBar.SetCurrentSegments.NegativeSegments", "Trying to set a negative number of segments! Clamping to 0.");
          currentNumSegments = 0;
        }
        else if (currentNumSegments > _CurrentSegments.Count) {
          DAS.Warn("SegmentedBar.SetCurrentSegments.OutOfRange", "Trying to set a more segments than the max! Clamping to max.");
          currentNumSegments = _CurrentSegments.Count;
        }

        bool hasOffImage = !string.IsNullOrEmpty(_OffImageSkinComponentId);
        bool hasOnImage = !string.IsNullOrEmpty(_OnImageSkinComponentId);
        if (hasOffImage ^ hasOnImage) {
          DAS.Warn("SegmentedBar.OnlyOneImageSet",
                   string.Format("{0} needs an id for the OFF and ON image to toggle between", this.name));
        }

        if (hasOffImage && hasOnImage) {
          for (int i = 0; i < _CurrentSegments.Count; i++) {
            CozmoImage img = _CurrentSegments[i].graphic as CozmoImage;
            if (img != null) {
              _CurrentSegments[i].isOn = true;
              if (i < currentNumSegments) {
                img.LinkedComponentId = _OnImageSkinComponentId;
              }
              else {
                img.LinkedComponentId = _OffImageSkinComponentId;
              }
              img.UpdateSkinnableElements();
              img.SetMaterialDirty();
            }
          }
        }
        else {
          for (int i = 0; i < _CurrentSegments.Count; i++) {
            if (i < currentNumSegments) {
              if (!_CurrentSegments[i].isOn) {
                _CurrentSegments[i].isOn = true;
              }
            }
            else {
              if (_CurrentSegments[i].isOn) {
                _CurrentSegments[i].isOn = false;
              }
            }
          }
        }
      }

      private Toggle CreateSegment() {
        // Potentially use a pool for this? Can we share the pool across 
        // bars that use the same toggle prefab?
        GameObject newSegment = UIManager.CreateUIElement(_SegmentPrefab, _SegmentContainer.transform);
        Toggle toggle = newSegment.GetComponent<Toggle>();
        toggle.isOn = false;

        // Bars are display only so make sure the toggles can't be messed with
        toggle.interactable = false;
        toggle.transition = Selectable.Transition.None;

        return toggle;
      }

      private void DeleteSegment(Toggle toggle) {
        // Potentially use a pool for this? Can we share the pool across 
        // bars that use the same toggle prefab?
        Destroy(toggle.gameObject);
      }
    }
  }
}

using UnityEngine;
using System.Collections;

public class PatternDisplay : MonoBehaviour {
  public CozmoCube[] cubes;

  [SerializeField]
  private RectTransform _NotFoundDisplay;
  
  [SerializeField]
  private BadgeDisplay _NewBadgeDisplay;

  private BlockPattern _Pattern;

  public BlockPattern Pattern {
    get {
      return _Pattern;
    }
    set {
      _Pattern = value;

      if (_NotFoundDisplay != null) {
        _NotFoundDisplay.gameObject.SetActive(_Pattern == null);
      }

      if (_Pattern != null) {
        for (int i = 0; i < cubes.Length && i < Pattern.Blocks.Count; i++) {
          // Show the cube
          cubes[i].gameObject.SetActive(true);

          // Set up the colors
          Color patternColor = new Color(0.2f, 0.8f, 1f);
          cubes[i].frontColor.ObjectColor = Pattern.Blocks[i].front ? patternColor : Color.gray;
          cubes[i].backColor.ObjectColor = Pattern.Blocks[i].back ? patternColor : Color.gray;
          cubes[i].leftColor.ObjectColor = Pattern.Blocks[i].left ? patternColor : Color.gray;
          cubes[i].rightColor.ObjectColor = Pattern.Blocks[i].right ? patternColor : Color.gray;

          // Update the cube's orientation depending on if the cube is facing cozmo
          cubes[i].SetOrientation(Pattern.Blocks[i].facing_cozmo);
        }

        if (_NewBadgeDisplay != null) {
          _NewBadgeDisplay.UpdateDisplayWithKey(_Pattern);
        }
      }
      else {
        // Hide all the cubes
        for (int i = 0; i < cubes.Length; i++) {
          cubes[i].gameObject.SetActive(false);
        }
        
        if (_NewBadgeDisplay != null) {
          _NewBadgeDisplay.HideDisplay();
        }
      }
    }
  }

  public void RemoveBadgeIfSeen() {
    if (_NewBadgeDisplay == null || _Pattern == null || !BadgeManager.DoesBadgeExistForKey(_Pattern)) {
      return;
    }

    Vector3[] worldCorners = new Vector3[4];
    RectTransform rectTransform = transform as RectTransform;
    rectTransform.GetWorldCorners(worldCorners);

    Rect viewportRect = new Rect(0f, 0f, 1f, 1f);
    bool isObjectOffscreen = false;
    foreach (Vector3 worldPos in worldCorners) {
      Vector3 screenPos = UIManager.GetUICamera().WorldToViewportPoint(worldPos);
      if (!viewportRect.Contains(screenPos)) {
        isObjectOffscreen = true;
        break;
      }
    }
    
    if (!isObjectOffscreen) {
      BadgeManager.TryRemoveBadge(_Pattern);
    }
  }
}

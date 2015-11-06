using UnityEngine;
using System.Collections;

public class PatternDisplay : MonoBehaviour {
  public CozmoCube[] cubes;

  [SerializeField]
  private RectTransform _notFoundDisplay;
  
  [SerializeField]
  private BadgeDisplay _newBadgeDisplay;

  private BlockPattern _pattern;
  public BlockPattern pattern {
    get {
      return _pattern;
    }
    set {
      _pattern = value;

      if (_notFoundDisplay != null) {
        _notFoundDisplay.gameObject.SetActive(_pattern == null);
      }

      if (_pattern != null){
        for (int i = 0; i < cubes.Length && i < pattern.blocks_.Count; i++) {
          // Show the cube
          cubes[i].gameObject.SetActive(true);

          // Set up the colors
          Color patternColor = new Color(0.2f, 0.8f, 1f);
          cubes[i].frontColor.ObjectColor = pattern.blocks_[i].front ? patternColor : Color.gray;
          cubes[i].backColor.ObjectColor = pattern.blocks_[i].back ? patternColor : Color.gray;
          cubes[i].leftColor.ObjectColor = pattern.blocks_[i].left ? patternColor : Color.gray;
          cubes[i].rightColor.ObjectColor = pattern.blocks_[i].right ? patternColor : Color.gray;

          // Update the cube's orientation depending on if the cube is facing cozmo
          cubes[i].SetOrientation(pattern.blocks_[i].facing_cozmo);
        }

        if (_newBadgeDisplay != null) {
          _newBadgeDisplay.UpdateDisplayWithKey(_pattern);
        }
      } else {
        // Hide all the cubes
        for (int i = 0; i < cubes.Length; i++) {
          cubes[i].gameObject.SetActive(false);
        }
        
        if (_newBadgeDisplay != null) {
          _newBadgeDisplay.HideDisplay();
        }
      }
    }
  }

  public void RemoveBadgeIfSeen() {
    if (_newBadgeDisplay  == null || _pattern == null || !BadgeManager.DoesBadgeExistForKey(_pattern)) {
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
      BadgeManager.TryRemoveBadge(_pattern);
    }
  }
}

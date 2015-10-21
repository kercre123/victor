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

      _notFoundDisplay.gameObject.SetActive(_pattern == null);

      if (_pattern != null){
        for (int i = 0; i < cubes.Length && i < pattern.blocks.Count; i++) {
          // Show the cube
          cubes[i].gameObject.SetActive(true);

          // Set up the colors
          cubes[i].frontColor.ObjectColor = pattern.blocks[i].front ? Color.green : Color.black;
          cubes[i].backColor.ObjectColor = pattern.blocks[i].back ? Color.green : Color.black;
          cubes[i].leftColor.ObjectColor = pattern.blocks[i].left ? Color.green : Color.black;
          cubes[i].rightColor.ObjectColor = pattern.blocks[i].right ? Color.green : Color.black;

          // Update the cube's orientation depending on if the cube is facing cozmo
          cubes[i].SetOrientation(pattern.blocks[i].facing_cozmo);
        }

        _newBadgeDisplay.UpdateDisplayWithKey(_pattern);
      }
      else {
        // Hide all the cubes
        for (int i = 0; i < cubes.Length; i++)
        {
          cubes[i].gameObject.SetActive(false);
        }

        _newBadgeDisplay.HideDisplay();
      }
    }
  }

}

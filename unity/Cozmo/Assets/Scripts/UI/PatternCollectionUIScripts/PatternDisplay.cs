using UnityEngine;
using System.Collections;

public class PatternDisplay : MonoBehaviour {

  public CozmoCube[] cubes;

  private BlockPattern _pattern;
  public BlockPattern pattern {
    get {
      return _pattern;
    }
    set {
      _pattern = value;
      for (int i = 0; i < cubes.Length && i < pattern.blocks.Count; i++) {
        cubes[i].frontColor.ObjectColor = pattern.blocks[i].front ? Color.green : Color.black;
        cubes[i].backColor.ObjectColor = pattern.blocks[i].back ? Color.green : Color.black;
        cubes[i].leftColor.ObjectColor = pattern.blocks[i].left ? Color.green : Color.black;
        cubes[i].rightColor.ObjectColor = pattern.blocks[i].right ? Color.green : Color.black;
      }
    }
  }
}

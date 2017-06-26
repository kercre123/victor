using Cozmo.UI;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Needs.UI {
  public class SegmentedBarAnimated : SegmentedBar {

    public void SetBoolAnimParam(string paramName, bool val) {
      for (int i = 0; i < _CurrentSegments.Count; i++) {
        Animator anim = _CurrentSegments[i].GetComponent<Animator>();
        anim.SetBool(paramName, val);
      }
    }

  }
}
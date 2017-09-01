using UnityEngine;

namespace Cozmo.UI {
  public class TooltipNeedsMeterData : TooltipGenericBaseDataComponent {

    [SerializeField]
    private Anki.Cozmo.NeedId _NeedId;

    protected override string GetBodyString() {
      // Get only based on which need ID
      Needs.NeedsValue needsVal = Needs.NeedsStateManager.Instance.GetCurrentDisplayValue(_NeedId);
      int index = (int)needsVal.Bracket;
      return _BodyLocKeys.Length > index ? Localization.Get(_BodyLocKeys[index]) : base.GetBodyString();
    }

  }

}
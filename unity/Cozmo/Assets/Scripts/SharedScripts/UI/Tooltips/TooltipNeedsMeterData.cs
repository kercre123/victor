using UnityEngine;

namespace Cozmo.UI {
  public class TooltipNeedsMeterData : TooltipGenericBaseDataComponent {

    [SerializeField]
    private Anki.Cozmo.NeedId _NeedId;

    protected override void HandlePointerDown(UnityEngine.EventSystems.BaseEventData data) {
      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.RewardBox)) {
        return;
      }
      base.HandlePointerDown(data);
    }

    protected override string GetBodyLocKey() {
      // Get only based on which need ID
      Needs.NeedsValue needsVal = Needs.NeedsStateManager.Instance.GetCurrentDisplayValue(_NeedId);
      int index = (int)needsVal.Bracket;
      return _BodyLocKeys.Length > index ? _BodyLocKeys[index] : base.GetBodyLocKey();
    }

  }

}
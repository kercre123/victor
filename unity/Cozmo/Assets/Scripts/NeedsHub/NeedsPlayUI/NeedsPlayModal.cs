using Anki.Cozmo;
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.Play.UI {
  public class NeedsPlayModal : BaseModal {

    [SerializeField]
    private RectTransform _MetersAnchor;

    [SerializeField]
    private NeedsMetersWidget _MetersWidgetPrefab;

    private NeedsMetersWidget _MetersWidget;

    public void InitializePlayModal() {
      NeedsStateManager nsm = NeedsStateManager.Instance;

      _MetersWidget = UIManager.CreateUIElement(_MetersWidgetPrefab.gameObject, _MetersAnchor).GetComponent<NeedsMetersWidget>();
      _MetersWidget.Initialize(dasParentDialogName: DASEventDialogName,
                               baseDialog: this,
                               focusOnMeter: NeedId.Play);

    }

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      ConstructDefaultFadeOpenAnimation(openAnimation);
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      ConstructDefaultFadeCloseAnimation(closeAnimation);
    }
  }
}
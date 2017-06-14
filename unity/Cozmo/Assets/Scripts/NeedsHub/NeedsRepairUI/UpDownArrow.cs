using UnityEngine;
using DG.Tweening;

namespace Cozmo.Repair.UI {
  public class UpDownArrow : MonoBehaviour {

    [SerializeField]
    private GameObject _RevealedUp = null;

    [SerializeField]
    private GameObject _RevealedDown = null;

    [SerializeField]
    private GameObject _MatchedUp = null;

    [SerializeField]
    private GameObject _MatchedDown = null;

    [SerializeField]
    private GameObject _MismatchedUp = null;

    [SerializeField]
    private GameObject _MismatchedDown = null;

    [SerializeField]
    private GameObject _CalibratedUp = null;

    [SerializeField]
    private GameObject _CalibratedDown = null;

    [SerializeField]
    private float _ScaleUpDuration = 0.25f;

    private void OnEnable() {
      transform.localScale = new Vector3(1f, 0f, 1f);
      transform.DOScale(Vector4.one, _ScaleUpDuration).SetEase(Ease.InQuad);
    }

    public void Reveal(ArrowInput arrow) {
      HideAll();
      _RevealedUp.SetActive(arrow == ArrowInput.Up);
      _RevealedDown.SetActive(arrow == ArrowInput.Down);
    }

    public void Matched(ArrowInput arrow) {
      HideAll();
      _MatchedUp.SetActive(arrow == ArrowInput.Up);
      _MatchedDown.SetActive(arrow == ArrowInput.Down);
    }

    public void Mismatched(ArrowInput arrow) {
      _MismatchedUp.SetActive(arrow == ArrowInput.Up);
      _MismatchedDown.SetActive(arrow == ArrowInput.Down);
    }

    public void Calibrated(ArrowInput arrow) {
      HideAll();
      _CalibratedUp.SetActive(arrow == ArrowInput.Up);
      _CalibratedDown.SetActive(arrow == ArrowInput.Down);
    }

    public void HideAll() {
      _RevealedUp.SetActive(false);
      _RevealedDown.SetActive(false);
      _MatchedUp.SetActive(false);
      _MatchedDown.SetActive(false);
      _MismatchedUp.SetActive(false);
      _MismatchedDown.SetActive(false);
      _CalibratedUp.SetActive(false);
      _CalibratedDown.SetActive(false);
    }
  }
}

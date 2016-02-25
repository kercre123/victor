using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class VizBehaviorBar : MonoBehaviour {

  [SerializeField]
  private Image _BarLeft;

  [SerializeField]
  private Image _BarRight;

  [SerializeField]
  private Text _Label;


  public void SetLabel(string label) {
    _Label.text = label;
  }

  public void SetValue(float valLeft, float valRight) {
    _BarLeft.fillAmount = valLeft;
    _BarRight.fillAmount = valRight;
  }

  public void SetColor(Color colorLeft, Color colorRight) {
    _BarLeft.color = colorLeft;
    _BarRight.color = colorRight;
  }
}

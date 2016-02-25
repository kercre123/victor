using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class VizEmotionBar : MonoBehaviour {

  [SerializeField]
  private Image _Bar;

  [SerializeField]
  private Text _Label;

	
  public void SetLabel(string label) {
    _Label.text = label;
  }

  public void SetValue(float val) {
    _Bar.fillAmount = (val + 1) * 0.5f;
  }
}

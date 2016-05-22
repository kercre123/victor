using UnityEngine;
using System.Collections;

namespace Vortex {

  public class PieSlice : MonoBehaviour {
    [SerializeField]
    private UnityEngine.UI.Text _Text;
    private int _Number;

    public int Number { get { return _Number; } }

    public void Init(float fillAmount, float angle, float textAngle, int num, Color imageColor) {
      // Image is set to Fill Radial 360 in the editor.
      UnityEngine.UI.Image image = this.GetComponent<UnityEngine.UI.Image>();
      image.fillAmount = fillAmount;
      image.color = imageColor;
      transform.localRotation = Quaternion.AngleAxis(angle, Vector3.forward);
      RectTransform textAnchor = _Text.GetComponent<RectTransform>();
      textAnchor.localRotation = Quaternion.AngleAxis(textAngle, Vector3.forward);
      _Number = num;
      _Text.text = num.ToString();
    }
  }

}

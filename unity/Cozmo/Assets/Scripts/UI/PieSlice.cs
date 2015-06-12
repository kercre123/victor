using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class PieSlice : MonoBehaviour {
	[SerializeField] Image image;
	[SerializeField] Text text;
	[SerializeField] RectTransform textAnchor;

	public int Number { get; private set; }

	public void Initialize(float fillAmount, float angle, int num, Color imageColor, Color textColor) {
		image.fillAmount = fillAmount;
		transform.localRotation = Quaternion.AngleAxis(angle, Vector3.forward);
		image.color = imageColor;
		Number = num;
		text.text = num.ToString();
		text.color = textColor;
		textAnchor.localRotation = Quaternion.AngleAxis(-fillAmount*180f, Vector3.forward);
	}

}

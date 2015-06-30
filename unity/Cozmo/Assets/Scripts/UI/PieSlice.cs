using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class PieSlice : MonoBehaviour {
	[SerializeField] Image image;
	[SerializeField] Text text;
	[SerializeField] RectTransform textAnchor;

	Outline textOutline;
	Material matCopy;

	public int Number { get; private set; }

	void Awake() {
		textOutline = text.gameObject.GetComponent<Outline>();
		matCopy = new Material(image.material);
		image.material = matCopy;
	}	

	public void Initialize(float fillAmount, float angle, float textAngleOffset, int num, Color imageColor, Color textColor, Color outlineColor) {
		image.fillAmount = fillAmount;
		transform.localRotation = Quaternion.AngleAxis(angle, Vector3.forward);
		textAnchor.localRotation = Quaternion.AngleAxis(textAngleOffset, Vector3.forward);
		Number = num;
		text.text = num.ToString();
		SetColors(imageColor, textColor, outlineColor);
	}

	public void SetColors(Color imageColor, Color textColor, Color outlineColor) {
		image.material.SetColor("_Color", imageColor);
		text.color = textColor;
		if(textOutline != null) textOutline.effectColor = outlineColor;
	}

}

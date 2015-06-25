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
		//image.color = imageColor;
		//MaterialPropertyBlock block = new MaterialPropertyBlock();
		//block.AddColor("_Color", imageColor);
		//image.canvasRenderer.SetColor(imageColor);
		image.material.SetColor("_Color", imageColor);
		Number = num;
		text.text = num.ToString();
		text.color = textColor;
		if(textOutline != null) textOutline.effectColor = outlineColor;
		textAnchor.localRotation = Quaternion.AngleAxis(textAngleOffset, Vector3.forward);
	}

}

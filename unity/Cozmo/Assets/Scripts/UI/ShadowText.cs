using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ShadowText : Text {

	public Color frontColor = Color.white;
	public Color shadowColor = Color.black;
	public Vector2 offset = new Vector2(-2, 2);

	string lastText = null;
	Color lastFrontColor = Color.white;
	Color lastShadowColor = Color.black;
	Vector2 lastOffset = new Vector2(-2, 2);

	Text frontText = null;

	protected override void OnEnable () {
		base.OnEnable();

		Transform childT = transform.Find("frontText");
		GameObject child = childT != null ? childT.gameObject : null;

		if(child == null) {
			child = new GameObject("frontText", typeof(RectTransform));
			child.layer = gameObject.layer;
			child.AddComponent<CanvasRenderer>();
			child.AddComponent<Text>();

			CozmoUtil.GetCopyOf<Text>(frontText, GetComponent<Text>());
			CozmoUtil.GetCopyOf<RectTransform>(frontText.rectTransform, rectTransform);
			frontText = child.GetComponent<Text>();
			frontText.rectTransform.SetParent(rectTransform, true);
		}
		else {
			frontText = child.GetComponent<Text>();
		}


		if(frontText == null) {
			Debug.LogError("ShadowText failed to create frontText.");
			return;
		}

		Refresh();
	}

	void Refresh() {
		color = shadowColor;

		frontText.text = text;
		frontText.color = frontColor;
		frontText.rectTransform.anchoredPosition = offset;

		lastText = text;
		lastFrontColor = frontColor;
		lastShadowColor = shadowColor;
		lastOffset = offset;
	}

	bool IsDirty() {
		if(lastText != text) return true;
		if(lastFrontColor != frontColor) return true;
		if(lastShadowColor != shadowColor) return true;
		if(lastOffset != offset) return true;

		return false;
	}
	
	void Update () {
		if(IsDirty()) {
			Refresh();
		}
	}
}

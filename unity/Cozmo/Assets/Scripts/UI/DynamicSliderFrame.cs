using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;

public class DynamicSliderFrame : MonoBehaviour, IPointerDownHandler, IPointerUpHandler, IDragHandler {
	[SerializeField] Slider slider;
	[SerializeField] Image clickableImage;

	RectTransform rTrans;
	RectTransform sliderTrans;
	EventTrigger sliderTrigger;
	Canvas canvas;
	CanvasScaler scaler;
	float screenScaleFactor = 1f;

	Vector2 Center {
		get {
			
			Vector2 center = Vector2.zero;
			
			Vector3[] corners = new Vector3[4];
			rTrans.GetWorldCorners(corners);

			if(corners == null || corners.Length == 0) return center;
			center = (corners[0] + corners[2]) * 0.5f;
			
			center = canvas.worldCamera.WorldToScreenPoint(center);
			
			return center * screenScaleFactor;
		}
	}

	void Awake() {
		rTrans = transform as RectTransform;
		sliderTrans = slider.transform as RectTransform;
		sliderTrigger = slider.gameObject.GetComponent<EventTrigger>();
		clickableImage = GetComponent<Image>();

		screenScaleFactor = 1f;

		canvas = GetComponentInParent<Canvas>();
		scaler = canvas.gameObject.GetComponent<CanvasScaler>();

		if(scaler != null) {
			screenScaleFactor = scaler.referenceResolution.y / Screen.height;
		}

		if(Screen.dpi > 0f) {
			float screenHeightInches = (float)Screen.height / (float)Screen.dpi;
			clickableImage.enabled = screenHeightInches > 3f;
		}
	}

	public void OnPointerDown(PointerEventData eventData) {

		Vector2 pointerPos = eventData.position * screenScaleFactor;
		sliderTrans.anchoredPosition = pointerPos - Center;
		sliderTrans.gameObject.SetActive(true);

		slider.OnPointerDown(eventData);
		if(sliderTrigger != null) sliderTrigger.OnPointerDown(eventData);
	}

	public void OnPointerUp(PointerEventData eventData) {
		slider.OnPointerUp(eventData);
		if(sliderTrigger != null) sliderTrigger.OnPointerUp(eventData);

		sliderTrans.gameObject.SetActive(false);
	}

	public void OnDrag(PointerEventData eventData) {
		slider.OnDrag(eventData);
		if(sliderTrigger != null) sliderTrigger.OnDrag(eventData);
	}
}

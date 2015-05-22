using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;

public class DynamicSliderFrame : MonoBehaviour, IPointerDownHandler, IPointerUpHandler, IDragHandler {
	[SerializeField] Slider slider;
	[SerializeField] RectTransform hintsToHide;
	[SerializeField] RectTransform hintsThatFollow;

	RectTransform rTrans;
	RectTransform sliderTrans;
	EventTrigger sliderTrigger;
	Canvas canvas;
	CanvasScaler scaler;
	float screenScaleFactor = 1f;

	bool dynamic = true;

	Vector2 Center {
		get {
			
			Vector2 center = Vector2.zero;
			
			Vector3[] corners = new Vector3[4];
			rTrans.GetWorldCorners(corners);

			if(corners == null || corners.Length == 0) return center;
			center = (corners[0] + corners[2]) * 0.5f;

			return RectTransformUtility.WorldToScreenPoint(canvas.worldCamera, center) * screenScaleFactor;
		}
	}

	void Awake() {
		rTrans = transform as RectTransform;
		sliderTrans = slider.transform as RectTransform;
		sliderTrigger = slider.gameObject.GetComponent<EventTrigger>();

		screenScaleFactor = 1f;

		canvas = GetComponentInParent<Canvas>();
		scaler = canvas.gameObject.GetComponent<CanvasScaler>();

		if(scaler != null) {
			screenScaleFactor = scaler.referenceResolution.y / Screen.height;
		}

		if(Screen.dpi > 0f) {
			float screenHeightInches = (float)Screen.height / (float)Screen.dpi;
			dynamic = screenHeightInches > CozmoUtil.SMALL_SCREEN_MAX_HEIGHT;
		}

		sliderTrans.gameObject.SetActive(!dynamic);

		if(hintsToHide != null) hintsToHide.gameObject.SetActive(dynamic);
	}


	void OnEnable() {
		if (hintsThatFollow != null) {
			hintsThatFollow.anchoredPosition = Vector2.zero;
			hintsThatFollow.gameObject.SetActive (true);
		}
	}

	void OnDisable() {
		if(dynamic) slider.gameObject.SetActive(false);
		if (hintsThatFollow != null) hintsThatFollow.gameObject.SetActive (false);
	}

	public void OnPointerDown(PointerEventData eventData) {
		if(dynamic) {
			Vector2 pointerPos = eventData.position * screenScaleFactor;
			sliderTrans.anchoredPosition = pointerPos - Center;
			if(hintsThatFollow != null) hintsThatFollow.anchoredPosition = sliderTrans.anchoredPosition;
			sliderTrans.gameObject.SetActive(true);
			if(hintsToHide != null) {
				hintsToHide.gameObject.SetActive(false);
			}
			slider.OnPointerDown(eventData);
			if(sliderTrigger != null) sliderTrigger.OnPointerDown(eventData);
		}
		else if(RectTransformUtility.RectangleContainsScreenPoint(sliderTrans, eventData.position, canvas.camera)) {
			slider.OnPointerDown(eventData);
			if(sliderTrigger != null) sliderTrigger.OnPointerDown(eventData);
		}
	}

	public void OnPointerUp(PointerEventData eventData) {

		if(dynamic) {
			slider.OnPointerUp(eventData);
			if(sliderTrigger != null) sliderTrigger.OnPointerUp(eventData);
			sliderTrans.gameObject.SetActive(false);

			if(hintsThatFollow != null) hintsThatFollow.anchoredPosition = Vector2.zero;
			if(hintsToHide != null) hintsToHide.gameObject.SetActive(true);
		}
		else if(RectTransformUtility.RectangleContainsScreenPoint(sliderTrans, eventData.position, canvas.camera)) {
			slider.OnPointerUp(eventData);
			if(sliderTrigger != null) sliderTrigger.OnPointerUp(eventData);
		}
	}

	public void OnDrag(PointerEventData eventData) {
		if(dynamic) {
			slider.OnDrag(eventData);
			if(sliderTrigger != null) sliderTrigger.OnDrag(eventData);
		}
		else if(RectTransformUtility.RectangleContainsScreenPoint(sliderTrans, eventData.position, canvas.camera)) {
			slider.OnDrag(eventData);
			if(sliderTrigger != null) sliderTrigger.OnDrag(eventData);
		}
	}
}

using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using UnityEngine.EventSystems;


[ExecuteInEditMode]
public class VirtualStick : MonoBehaviour, IPointerDownHandler, IDragHandler, IPointerUpHandler {
	[SerializeField] bool dynamic = true; //recenter the stick from touchdown position
	[SerializeField] RectTransform bg = null;
	[SerializeField] RectTransform stick = null;

	[SerializeField] RectTransform deadZoneRect = null;

	[SerializeField] RectTransform capTop = null;
	[SerializeField] RectTransform capBottom = null;
	[SerializeField] RectTransform capLeft = null;
	[SerializeField] RectTransform capRight = null;

	[SerializeField] RectTransform originMarker = null;

	[SerializeField] Image bgDefaultMode = null;
	[SerializeField] Image bgUpMode = null;
	[SerializeField] Image bgDownMode = null;
	[SerializeField] Image bgSideMode = null;

	[SerializeField] Image stickImageThrown = null;
	[SerializeField] Image stickImageNeutral = null;

	[SerializeField] bool clampRadially = true;

	[SerializeField] Vector2 deadZone = new Vector2(0.01f, 0.01f);
	[SerializeField] bool deadZoneRadial = false;

	[SerializeField] float verticalAxisSnapAngle = 0f;
	[SerializeField] float horizontalAxisSnapAngle = 0f;

	[SerializeField] float verticalModeEntryAngle = 0f;
	[SerializeField] float horizontalModeEntryAngle = 0f;

	[SerializeField] bool clearHorizontal = true;
	[SerializeField] bool clearVertical = true;

	[SerializeField] float bgWidthInches = -1f;
	[SerializeField] float bgHeightInches = -1f;

	[SerializeField] float stickWidthInches = -1f;
	[SerializeField] float stickHeightInches = -1f;

	[SerializeField] bool allowAxisSwipes = false;
	[SerializeField] bool allowPreciseSwipes = false;

	[SerializeField] float swipeMinSpeed = 4f;

	[SerializeField] bool horizontalOnly = false;
	[SerializeField] bool verticalOnly = false;

	[SerializeField] bool scaleToScreen = false;

	[SerializeField] float[] swipeVerticalMagnitudes = { 1f };
	[SerializeField] float[] swipeHorizontalMagnitudes = { 0.25f, 0.5f, 1f };

	int swipeIndex = 0;

	float radius;
	//Canvas canvas;
	RectTransform rTrans;
	ScreenOrientation orientation;
	float lastScreenWidth = 0f;
	float lastScreenHeight = 0f;

	bool pressed = false;
	float pressedTime = 0f;

	Vector2 touchDownValue = Vector2.zero;
	Vector2 swipedDirection = Vector2.zero;
	//bool disallowSwipeForThisTouch = false;

	public bool UpModeEngaged { get; private set; }
	public bool DownModeEngaged { get; private set; }
	public bool SideModeEngaged { get; private set; }

	public bool IsPressed { get { return pressed; } }
	public float PressedTime { get { return pressedTime; } }

	public Vector2 StickCenterOnScreen {
		get {
			return ZoneCenter + bg.anchoredPosition;
		}
	}

	public Vector2 ZoneCenter {
		get {
			return (Vector2)rTrans.position + rTrans.rect.center;
		}
	}

	public Vector2 JoystickData {
		get {
			Vector2 res = stick.anchoredPosition - bg.anchoredPosition;

			if(clampRadially && !horizontalOnly && !verticalOnly && !SideModeEngaged) {
				res = res / radius;
			}
			else {
				if(bg.rect.width > 0f) res.x /= bg.rect.width * 0.5f;
				if(bg.rect.height > 0f) res.y /= bg.rect.height * 0.5f;
			}

			//Debug.Log("res.x("+res.x+") deadZone.x("+deadZone.x+")");

			if(deadZoneRadial && !horizontalOnly && !verticalOnly && !SideModeEngaged) {
				if(res.magnitude < deadZone.x) {
					res = Vector2.zero;
				}
			}
			else {
				if(Mathf.Abs(res.x) < deadZone.x) {

					res.x = 0f;
				}
				
				if(Mathf.Abs(res.y) < deadZone.y) {
					res.y = 0f;
				}
			}

			if(horizontalOnly || SideModeEngaged) {
				res.y = 0f;
			}

			if(verticalOnly) {
				res.x = 0f;
			}

			if(UpModeEngaged) {
				res.y = Mathf.Max(0f, res.y);
			}
			else if(DownModeEngaged) {
				res.y = Mathf.Min(0f, res.y);
			}

			if(verticalAxisSnapAngle > 0f && res.sqrMagnitude > 0f) {
				float angleFromUp = Vector2.Angle(Vector2.up, res.normalized);

				if(angleFromUp <= verticalAxisSnapAngle) {
					res = Vector2.up * res.magnitude;
				}
				else if(angleFromUp >= 180f - verticalAxisSnapAngle) {
					res = -Vector2.up * res.magnitude;
				}
			}

			if(horizontalAxisSnapAngle > 0f && res.sqrMagnitude > 0f) {
				float angleFromRight = Vector2.Angle(Vector2.right, res.normalized);
				
				if(angleFromRight <= horizontalAxisSnapAngle) {
					res = Vector2.right * res.magnitude;
				}
				else if(angleFromRight >= 180f - horizontalAxisSnapAngle) {
					res = -Vector2.right * res.magnitude;
				}
			}


			return res;
		}
	}

	public float Horizontal {
		get { 
			return JoystickData.x; 
		}
	}

	public float Vertical {
		get { 
			return JoystickData.y; 
		}
	}

	void Awake() {
		//canvas = GetComponentInParent<Canvas>();
		rTrans = transform as RectTransform;
	}

	void OnEnable() {
		//canvas = GetComponentInParent<Canvas>();
		rTrans = transform as RectTransform;

		ResizeToScreen();
		if(!Application.isPlaying) return;
		//Debug.Log ("Joystick Awake startPosition(" + startPosition + ") stick.position("+stick.position+") stick.anchoredPosition("+stick.anchoredPosition+")");
		bg.gameObject.SetActive(!dynamic);
		stick.gameObject.SetActive(!dynamic);

		swipeIndex = 0;
	}

	void RefreshRadius() {
		if(verticalOnly) {
			radius = bg.rect.height * 0.5f;
		}
		else if(horizontalOnly) {
			radius = bg.rect.width * 0.5f;
		}
		else {
			radius = Mathf.Min(bg.rect.width, bg.rect.height) * 0.5f;
		}
	}

	void ResizeToScreen() {
		if(Screen.dpi == 0f) return;

		if(scaleToScreen) {
			rTrans.anchoredPosition = Vector2.zero;
			float length = Mathf.Min(Screen.height, Screen.width);
			rTrans.sizeDelta = new Vector2(length, length);
		}

		if(bgWidthInches > 0f && bgHeightInches > 0f) {
			Vector3 size = bg.sizeDelta;
			size.x = Mathf.Clamp(Screen.dpi * bgWidthInches, 0f, rTrans.rect.width * 0.75f);
			size.y = Mathf.Clamp(Screen.dpi * bgHeightInches, 0f, rTrans.rect.height * 0.75f);
			bg.sizeDelta = size;
		}

		if(stickWidthInches > 0f && stickHeightInches > 0f) {
			Vector3 size = stick.sizeDelta;
			size.x = Mathf.Clamp(Screen.dpi * stickWidthInches, 0f, Screen.width);
			size.y = Mathf.Clamp(Screen.dpi * stickHeightInches, 0f, Screen.height);;
			stick.sizeDelta = size;

			if(capTop != null) capTop.sizeDelta = size;
			if(capBottom != null) capBottom.sizeDelta = size;
			if(capLeft != null) capLeft.sizeDelta = size;
			if(capRight != null) capRight.sizeDelta = size;

			if(originMarker != null) originMarker.sizeDelta = size;

			if(deadZoneRect != null) {
				deadZoneRect.sizeDelta = size;
			}

		}

		bg.anchoredPosition = Vector3.zero;
		stick.anchoredPosition = Vector3.zero;

		float height = bg.rect.height;
		float width = bg.rect.width;

		if(capTop != null) capTop.anchoredPosition = Vector2.up * height * 0.5f;
		if(capBottom != null) capBottom.anchoredPosition = -Vector2.up * height * 0.5f;
		if(capLeft != null) capLeft.anchoredPosition = -Vector2.right * width * 0.5f;
		if(capRight != null) capRight.anchoredPosition = Vector2.right * width * 0.5f;

		if(originMarker != null) originMarker.anchoredPosition = Vector2.zero;

		if(deadZoneRect != null) {
			Vector3 size = deadZoneRect.sizeDelta;
			if(deadZoneRadial) {
				size.x = size.y = deadZone.x * width;
			}
			else {
				if(deadZone.x > 0f && !verticalOnly) size.x = deadZone.x * width;
				if(deadZone.y > 0f && !horizontalOnly) size.y = deadZone.y * height;
			}

			deadZoneRect.sizeDelta = size;
		}

		RefreshRadius();

		lastScreenWidth = Screen.width;
		lastScreenHeight = Screen.height;
		orientation = Screen.orientation;
	}

	void Update() {
		if(	Screen.orientation != orientation || lastScreenWidth != Screen.width || lastScreenHeight != Screen.height ) {
			ResizeToScreen();
		}

		if(!Application.isPlaying) return;

		if(pressed) {
			pressedTime += Time.deltaTime;
		}
		else {
			pressedTime = 0f;

			//if we've allowed a swipe to override stick, then snap it here
			if(swipedDirection.sqrMagnitude > 0f) {

				float[] mags = swipeVerticalMagnitudes;
				if(Vector2.Dot(swipedDirection, Vector2.right) != 0f) {
					mags = swipeHorizontalMagnitudes;
				}

				float mag = 1f;
				swipeIndex = Mathf.Clamp(swipeIndex, 0, mags.Length-1);
				mag = mags[swipeIndex];

				Vector3 anchorPos = bg.anchoredPosition + swipedDirection * radius * mag;
				stick.anchoredPosition = anchorPos;
			}
			else {
				swipeIndex = 0;
			}
		}

		RefreshStickImages();
		RefreshBGImages();
		RefreshCaps();

	}

	void RefreshStickImages() {
		if(stickImageThrown == null) return;
		if(stickImageNeutral == null) return;

		Vector2 throwVector = JoystickData;

		bool thrown = throwVector.sqrMagnitude > 0f;
		if(horizontalOnly || SideModeEngaged) thrown = throwVector.x != 0f;
		if(verticalOnly) thrown = throwVector.y != 0f;

		stickImageThrown.enabled = thrown;
		stickImageNeutral.enabled = !thrown;

		//Debug.Log("RefreshStickImages thrown("+thrown+") throwVector("+throwVector.x +", "+throwVector.y+") SideModeEngaged("+SideModeEngaged+")");

		if(thrown) {
			float angle = Vector2.Angle(Vector3.up, throwVector);
			if(Vector2.Dot(Vector2.right, throwVector) > 0f) {
				angle = -angle;
			}

			stickImageThrown.rectTransform.localRotation = Quaternion.AngleAxis(angle, Vector3.forward);
		}
	}

	void RefreshBGImages() {
		if(bgDefaultMode != null) bgDefaultMode.gameObject.SetActive(!UpModeEngaged && !DownModeEngaged && !SideModeEngaged);
		if(bgUpMode != null) bgUpMode.gameObject.SetActive(UpModeEngaged);
		if(bgDownMode != null) bgDownMode.gameObject.SetActive(DownModeEngaged);
		if(bgSideMode != null) bgSideMode.gameObject.SetActive(SideModeEngaged);
	}

	void RefreshCaps() {

		if(!UpModeEngaged && !DownModeEngaged && !SideModeEngaged) {
			if(capTop != null)
				capTop.gameObject.SetActive(true);
			if(capBottom != null)
				capBottom.gameObject.SetActive(true);
			if(capLeft != null)
				capLeft.gameObject.SetActive(true);
			if(capRight != null)
				capRight.gameObject.SetActive(true);
		}
		else {
			if(capTop != null)
				capTop.gameObject.SetActive(UpModeEngaged);
			if(capBottom != null)
				capBottom.gameObject.SetActive(DownModeEngaged);
			if(capLeft != null)
				capLeft.gameObject.SetActive(SideModeEngaged);
			if(capRight != null)
				capRight.gameObject.SetActive(SideModeEngaged);
		}
	}

	public void OnPointerDown(PointerEventData eventData) {

		if(dynamic) {
			bg.anchoredPosition = eventData.position - ZoneCenter;
			stick.anchoredPosition = bg.anchoredPosition;
		}

		bg.gameObject.SetActive(true);
		stick.gameObject.SetActive(true);

		ProcessStick(eventData);
		pressed = true;
		pressedTime = 0f;

		//if this is first touch after a swipe, prevent swipe input for this touch
		//disallowSwipeForThisTouch = swipedDirection.sqrMagnitude > 0f;
		//swipedDirection = Vector2.zero;
		touchDownValue = JoystickData;

		UpModeEngaged = false;
		DownModeEngaged = false;
		SideModeEngaged = false;

		//Debug.Log ("Joystick OnPointerDown eventData.position(" + eventData.position + ") ZoneCenter("+ZoneCenter+") disallowSwipeForThisTouch("+disallowSwipeForThisTouch+")");
	}

	public void OnDrag(PointerEventData eventData) {
		ProcessStick(eventData);
		CheckForModeEngage();
	}

	public void CheckForModeEngage() {
		if(verticalModeEntryAngle <= 0f && horizontalModeEntryAngle <= 0f)
			return;

		if(UpModeEngaged)
			return;
		if(DownModeEngaged)
			return;
		if(SideModeEngaged)
			return;

		Vector2 dragThusFar = JoystickData - touchDownValue;
		if(dragThusFar.sqrMagnitude <= 0f)
			return;

		dragThusFar.Normalize();
		float angleFromUp = Vector2.Angle(Vector2.up, dragThusFar);

		if(angleFromUp <= verticalModeEntryAngle) {
			UpModeEngaged = true;
			return;
		}

		if(angleFromUp >= 180f - verticalModeEntryAngle) {
			DownModeEngaged = true;
			return;
		}

		if(angleFromUp >= 90f - horizontalModeEntryAngle && angleFromUp <= 90f + horizontalModeEntryAngle) {
			SideModeEngaged = true;
			return;
		}
	}

	public void OnPointerUp(PointerEventData eventData) {
		ProcessStick(eventData);
		ConsiderSwipes(eventData.position);

		Vector2 throwVector = stick.anchoredPosition - bg.anchoredPosition;

		//if(dynamic) bg.anchoredPosition = Vector3.zero;

		if(clearHorizontal) {
			throwVector.x = 0f;
		}

		if(clearVertical) {
			throwVector.y = 0f;
		}

		stick.anchoredPosition = bg.anchoredPosition + throwVector;

		pressed = false;
		pressedTime = 0f;
		bool swiped = swipedDirection.sqrMagnitude > 0f;
		bg.gameObject.SetActive(!dynamic || swiped);
		stick.gameObject.SetActive(!dynamic || swiped);
		//Debug.Log ("Joystick OnPointerUp StickCenterOnScreen(" + StickCenterOnScreen + ") stick.anchoredPosition("+stick.anchoredPosition+")");
	}

	void ConsiderSwipes(Vector2 pos) {
		Vector3 oldSwipe = swipedDirection;
		swipedDirection = Vector2.zero;
		//if(disallowSwipeForThisTouch) return;
		if(!allowAxisSwipes && !allowPreciseSwipes) return;

		if(pressedTime <= 0f) {
			swipeIndex = 0;
			return;
		}
		if(pressedTime > 1f) {
			swipeIndex = 0;
			return;
		}

		Vector2 delta = JoystickData - touchDownValue;
		float distance = delta.magnitude;

		if(distance < swipeMinSpeed * pressedTime) {
			swipeIndex = 0;
			return;
		}

		swipedDirection = delta.normalized;

		//if our swipe dir has changed, start our stepwise magnitude index over
		if(oldSwipe.sqrMagnitude > 0f) {
			swipeIndex++;

			if(Vector2.Angle(oldSwipe, swipedDirection) > 10f) {
				swipeIndex = 0;
			}

//			&& Vector2.Dot(oldSwipe, swipedDirection) < 0f) {
//			swipedDirection = Vector2.zero;
		}

		if(allowPreciseSwipes && !horizontalOnly && !verticalOnly) return;

		float dotUp = Vector2.Dot(swipedDirection, Vector2.up);

		if(verticalOnly) {
			if(dotUp >= 0f) {
				swipedDirection = Vector2.up;
			}
			else {
				swipedDirection = -Vector2.up;
			}
			return;
		}

		float dotRight = Vector2.Dot(swipedDirection, Vector2.right);

		if(horizontalOnly || SideModeEngaged) {
			if(dotRight >= 0f) {
				swipedDirection = Vector2.right;
			}
			else {
				swipedDirection = -Vector2.right;
			}
			return;
		}

		if(Mathf.Abs(dotUp) >= Mathf.Abs(dotRight)) {
			if(dotUp >= 0f) {
				swipedDirection = Vector2.up;
			}
			else { 
				swipedDirection = -Vector2.up;
			}
		}
		else if(dotRight >= 0f) {
			swipedDirection = Vector2.right;
		}
		else {
			swipedDirection = -Vector2.right;
		}
	}

	void ProcessStick(PointerEventData evnt) {
		Vector2 delta = Vector2.zero;
		delta = evnt.position - StickCenterOnScreen;
		delta = Vector3.ClampMagnitude(delta, radius);

		if(verticalOnly) {
			delta.x = 0f;
		}

		if(UpModeEngaged) {
			delta.y = Mathf.Max(0f, delta.y);
		}

		if(DownModeEngaged) {
			delta.y = Mathf.Min(0f, delta.y);
		}

		if(horizontalOnly || SideModeEngaged) {
			delta.y = 0f;
		}

		stick.anchoredPosition = bg.anchoredPosition + delta;
		//Debug.Log ("Joystick evnt.position(" + evnt.position + ") radius(" + radius + ") delta(" + delta + ") bg.anchoredPosition("+bg.anchoredPosition+") stick.anchoredPosition("+stick.anchoredPosition+")");
	}
}

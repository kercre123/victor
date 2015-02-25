using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using UnityEngine.EventSystems;

[ExecuteInEditMode]
public class VirtualStick : MonoBehaviour, IPointerDownHandler, IDragHandler, IPointerUpHandler {

#region INSPECTOR FIELDS
	[SerializeField] bool dynamic = true; //recenter the stick from touchdown position
	[SerializeField] RectTransform bg = null;
	[SerializeField] RectTransform stick = null;
	[SerializeField] RectTransform deadZoneRect = null;
	[SerializeField] RectTransform capTop = null;
	[SerializeField] RectTransform capBottom = null;
	[SerializeField] RectTransform capLeft = null;
	[SerializeField] RectTransform capRight = null;
	[SerializeField] RectTransform originMarker = null;
	[SerializeField] RectTransform bgDefaultMode = null;
	[SerializeField] RectTransform bgUpMode = null;
	[SerializeField] RectTransform bgDownMode = null;
	[SerializeField] RectTransform bgSideMode = null;
	[SerializeField] Image stickImageThrown = null;
	[SerializeField] Image stickImageNeutral = null;
	[SerializeField] bool clampRadially = true;
	[SerializeField] Vector2 deadZone = new Vector2(0.01f, 0.01f);
	[SerializeField] bool deadZoneRadial = false;
	[SerializeField] bool scaleMagFromDeadZone = true;
	[SerializeField] float horClampThrow = 1f;
	[SerializeField] float verticalAxisSnapAngle = 0f;
	[SerializeField] float horizontalAxisSnapAngle = 0f;
	[SerializeField] float verticalModeEntryAngle = 0f;
	[SerializeField] float horizontalModeEntryAngle = 0f;
	[SerializeField] float verticalModeMaxAngleAllowance = 170f;
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
	[SerializeField] TextAnchor smallScreenAnchorType = TextAnchor.MiddleCenter;
	[SerializeField] Vector2 smallScreenAnchorPos = Vector2.zero;
#endregion

#region PROPERTIES
	public float MaxAngle { get { return verticalModeMaxAngleAllowance; } }
	public Vector2 SwipedDirection { get; private set; }
	public bool SwipeRequest { get; private set; }
	public bool SwipeActive { get { return SwipedDirection.sqrMagnitude > 0f; } }
	public int SwipeIndex { get; private set; }
	public bool UpModeEngaged { get; private set; }
	public bool DownModeEngaged { get; private set; }
	public bool SideModeEngaged { get; private set; }
	public bool IsPressed { get; private set; }
	public float PressedTime { get; private set; }
	public float Horizontal {
		get { 
			
			float hor = JoystickData.x;
			if(Mathf.Abs(hor) > horClampThrow) hor = horClampThrow * (hor >= 0f ? 1f : -1f);
			
			return hor; 
		}
	}
	public float Vertical {
		get { 
			return JoystickData.y; 
		}
	}
	Vector2 StickCenterOnScreen {
		get {
			return ZoneCenter + bg.anchoredPosition;
		}
	}
	Vector2 ZoneCenter {
		get {
			return (Vector2)rTrans.position + rTrans.rect.center;
		}
	}
	Vector2 JoystickData {
		get {
			Vector2 res = stick.anchoredPosition;
			
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
				else if(scaleMagFromDeadZone && deadZone.x > 0f) {
					//smooth scale magnitude from deadzone to full throw
					res = res.normalized * Mathf.Lerp(0f, 1f, (res.magnitude - deadZone.x) / (1f - deadZone.x));
				}
			}
			else {
				if(Mathf.Abs(res.x) < deadZone.x) {
					res.x = 0f;
				}
				else if(scaleMagFromDeadZone && deadZone.x > 0f) {
					//smooth scale magnitude from deadzone to full throw
					res.x = (res.x >= 0f ? 1f : -1f) * Mathf.Lerp(0f, 1f, (Mathf.Abs(res.x) - deadZone.x) / (1f - deadZone.x));
				}
				
				if(Mathf.Abs(res.y) < deadZone.y) {
					res.y = 0f;
				}
				else if(scaleMagFromDeadZone && deadZone.y > 0f) {
					//smooth scale magnitude from deadzone to full throw
					res.y = (res.y >= 0f ? 1f : -1f) * Mathf.Lerp(0f, 1f, (Mathf.Abs(res.y) - deadZone.y) / (1f - deadZone.y));
				}
			}
			
			return res;
		}
	}
#endregion

#region MISC MEMBERS
	float radius;
	RectTransform rTrans;
	ScreenOrientation orientation;
	float lastScreenWidth = 0f;
	float lastScreenHeight = 0f;
	Vector2 touchDownValue = Vector2.zero;
	bool wasClockwise = false;
	bool wasCounterClockwise = false;
	Vector2 oldSwipe;
#endregion

#region COMPONENT CALLBACKS
	void Awake() {
		//canvas = GetComponentInParent<Canvas>();
		rTrans = transform as RectTransform;
	}

	void OnEnable() {
		ResizeToScreen();
		if(!Application.isPlaying) return;
		//Debug.Log ("Joystick Awake startPosition(" + startPosition + ") stick.position("+stick.position+") stick.anchoredPosition("+stick.anchoredPosition+")");
		bg.gameObject.SetActive(!dynamic);
		stick.gameObject.SetActive(!dynamic);

		SwipeIndex = 0;
	}

	void Update() {
		if(	Screen.orientation != orientation || lastScreenWidth != Screen.width || lastScreenHeight != Screen.height ) {
			ResizeToScreen();
		}
		
		if(!Application.isPlaying) return;
		
		if(IsPressed) {
			PressedTime += Time.deltaTime;
		}
		else {
			PressedTime = 0f;
			
			//if we've allowed a swipe to override stick, then snap it here
			if(SwipeActive) {

				float[] mags = swipeVerticalMagnitudes;
				if(Vector2.Dot(SwipedDirection, Vector2.right) != 0f) {
					mags = swipeHorizontalMagnitudes;
				}

				float mag = 1f;
				SwipeIndex = Mathf.Clamp(SwipeIndex, 0, mags.Length-1);
				mag = mags[SwipeIndex];
				
				Vector2 pos = SwipedDirection * radius * mag;
				stick.anchoredPosition = pos;
			}
			else {
				SwipeIndex = 0;
			}
		}
		
		RefreshStickImages();
		RefreshBGImages();
		RefreshCaps();
		
	}
#endregion

#region PRIVATE METHODS
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

		float screenHeightInches = (float)Screen.height / (float)Screen.dpi;

		if(scaleToScreen) {
			rTrans.anchoredPosition = Vector2.zero;
			float length = Mathf.Min(Screen.height, Screen.width);
			rTrans.sizeDelta = new Vector2(length, length);
		}

		if(bgWidthInches > 0f && bgHeightInches > 0f) {
			Vector3 size = bg.sizeDelta;
			size.x = Mathf.Clamp(Screen.dpi * bgWidthInches, 0f, rTrans.rect.width);// * 0.75f);
			size.y = Mathf.Clamp(Screen.dpi * bgHeightInches, 0f, rTrans.rect.height);// * 0.75f);
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

		if(Application.isPlaying && screenHeightInches < 3f) {
			dynamic = false;

			Vector2 anchor = new Vector2(0.5f,0.5f);
			switch(smallScreenAnchorType) {
				case TextAnchor.UpperLeft:
					anchor.x = 0f;
					anchor.y = 1f;
					break;
				case TextAnchor.UpperCenter:
					anchor.y = 1f;
					break;
				case TextAnchor.UpperRight:
					anchor.x = 1f;
					anchor.y = 1f;
					break;
				case TextAnchor.MiddleLeft:
					anchor.x = 0f;
					break;
				case TextAnchor.MiddleCenter:
					break;
				case TextAnchor.MiddleRight:
					anchor.x = 1f;
					break;
				case TextAnchor.LowerLeft:
					anchor.x = 0f;
					anchor.y = 0f;
					break;
				case TextAnchor.LowerCenter:
					anchor.y = 0f;
					break;
				case TextAnchor.LowerRight:
					anchor.x = 1f;
					anchor.y = 0f;
					break;
			}

			bg.anchorMin = anchor;
			bg.anchorMax = anchor;
			bg.pivot = anchor;
			bg.anchoredPosition = smallScreenAnchorPos;
		}

		//Debug.Log("screenHeightInches("+screenHeightInches+") dynamic("+dynamic+")");

		stick.anchoredPosition = Vector2.zero;
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

	void ConsiderSwipe(Vector2 pos, bool underway) {
		SwipedDirection = Vector2.zero;
		if(!allowAxisSwipes && !allowPreciseSwipes) return;
		
		if(PressedTime <= 0f) {
			return;
		}
		if(PressedTime > 1f) {
			return;
		}
		
		Vector2 delta = JoystickData - touchDownValue;
		float distance = delta.magnitude;
		
		if(distance < swipeMinSpeed * PressedTime) return;

		SwipedDirection = delta.normalized;
		
		//if our swipe dir has changed, start our stepwise magnitude index over
		if(!underway && oldSwipe.sqrMagnitude > 0f) {
			SwipeIndex++;
			
			if(Vector2.Angle(oldSwipe, SwipedDirection) > 10f) {
				SwipeIndex = 0;
			}
		}
		
		if(allowPreciseSwipes && !horizontalOnly && !verticalOnly) return;
		
		float dotUp = Vector2.Dot(SwipedDirection, Vector2.up);
		
		if(verticalOnly) {
			if(dotUp >= 0f) {
				SwipedDirection = Vector2.up;
			}
			else {
				SwipedDirection = -Vector2.up;
			}
			return;
		}
		
		float dotRight = Vector2.Dot(SwipedDirection, Vector2.right);
		
		if(horizontalOnly || SideModeEngaged) {
			if(dotRight >= 0f) {
				SwipedDirection = Vector2.right;
			}
			else {
				SwipedDirection = -Vector2.right;
			}
			return;
		}
		
		if(Mathf.Abs(dotUp) >= Mathf.Abs(dotRight)) {
			if(dotUp >= 0f) {
				SwipedDirection = Vector2.up;
			}
			else { 
				SwipedDirection = -Vector2.up;
			}
		}
		else if(dotRight >= 0f) {
			SwipedDirection = Vector2.right;
		}
		else {
			SwipedDirection = -Vector2.right;
		}
	}

	void ProcessStick(PointerEventData evnt) {
		Vector2 delta = Vector2.zero;
		delta = evnt.position - StickCenterOnScreen;

		if(clampRadially) {
			delta = Vector3.ClampMagnitude(delta, radius);
		}
		else {
			delta.x = Mathf.Clamp(Mathf.Abs(delta.x), 0f, bg.rect.width * 0.5f) * (delta.x >= 0f ? 1f : -1f);
			delta.y = Mathf.Clamp(Mathf.Abs(delta.y), 0f, bg.rect.height * 0.5f) * (delta.y >= 0f ? 1f : -1f);
		}

		if(delta.sqrMagnitude <= 0f) {
			stick.anchoredPosition = Vector2.zero;
			return;
		}

		if(horizontalOnly || SideModeEngaged) {
			delta.y = 0f;
		}
		
		if(verticalOnly) {
			delta.x = 0f;
		}
		
		if(UpModeEngaged) {

			if(verticalModeMaxAngleAllowance > 0f) {
				float angle = Vector2.Angle(Vector2.up, delta.normalized);

				if(angle > 90f && !wasClockwise && !wasCounterClockwise) {
					wasClockwise = delta.x > 0f;
					wasCounterClockwise = delta.x < 0f;
	 			}
				else if(angle <= 90f) { //release the rot direction locks if we have returned to the top half of circle
					wasClockwise = false;
					wasCounterClockwise = false;
				}

				if(angle > verticalModeMaxAngleAllowance) {
					float signedAngle = verticalModeMaxAngleAllowance * (wasClockwise ? -1f : 1f);
					delta = Quaternion.AngleAxis(signedAngle, Vector3.forward) * Vector2.up * delta.magnitude;
				}
			}
			else {
				delta.y = Mathf.Max(0f, delta.y);
			}
		}
		else if(DownModeEngaged) {

			if(verticalModeMaxAngleAllowance > 0f) {
				float angle = Vector2.Angle(-Vector2.up, delta.normalized);

				if(angle > 90f && !wasClockwise && !wasCounterClockwise) {
					wasClockwise = delta.x < 0f;
					wasCounterClockwise = delta.x > 0f;
				}
				else if(angle <= 90f) { //release the rot direction locks if we have returned to the bottom half of circle
					wasClockwise = false;
					wasCounterClockwise = false;
				}

				if(angle > verticalModeMaxAngleAllowance) {
					float signedAngle = verticalModeMaxAngleAllowance * (wasClockwise ? -1f : 1f);
					delta = Quaternion.AngleAxis(signedAngle, Vector3.forward) * -Vector2.up * delta.magnitude;
				}
			}
			else {
				delta.y = Mathf.Min(0f, delta.y);
			}
		}
		
		if(verticalAxisSnapAngle > 0f) {
			float angleFromUp = Vector2.Angle(Vector2.up, delta.normalized);
			
			if(angleFromUp <= verticalAxisSnapAngle) {
				delta = Vector2.up * delta.magnitude;
			}
			else if(angleFromUp >= 180f - verticalAxisSnapAngle) {
				delta = -Vector2.up * delta.magnitude;
			}
		}
		
		if(horizontalAxisSnapAngle > 0f) {
			float angleFromRight = Vector2.Angle(Vector2.right, delta.normalized);
			
			if(angleFromRight <= horizontalAxisSnapAngle) {
				delta = Vector2.right * delta.magnitude;
			}
			else if(angleFromRight >= 180f - horizontalAxisSnapAngle) {
				delta = -Vector2.right * delta.magnitude;
			}
		}

		stick.anchoredPosition = delta;
		//Debug.Log ("Joystick evnt.position(" + evnt.position + ") radius(" + radius + ") delta(" + delta + ") bg.anchoredPosition("+bg.anchoredPosition+") stick.anchoredPosition("+stick.anchoredPosition+")");
	}
	
	void CheckForModeEngage() {
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
#endregion

#region PUBLIC METHODS
	public void EndSwipe() {

		SwipedDirection = Vector2.zero;
		SwipeRequest = false;
		SwipeIndex = 0;
		stick.anchoredPosition = Vector2.zero;
	}

	public void AbsorbSwipeRequest() {
		Debug.Log("AbsorbSwipeRequest()");
		SwipeRequest = false;
	}

	public void OnPointerDown(PointerEventData eventData) {
		
		if(dynamic) {
			bg.anchoredPosition = eventData.position - ZoneCenter;
			stick.anchoredPosition = Vector2.zero;
		}
		
		bg.gameObject.SetActive(true);
		stick.gameObject.SetActive(true);

		oldSwipe = SwipedDirection;
		if(SwipedDirection.sqrMagnitude == 0f) SwipeIndex = 0;
		SwipeRequest = false;
		IsPressed = true;
		PressedTime = 0f;
		
		ProcessStick(eventData);
		
		touchDownValue = JoystickData;
		
		UpModeEngaged = false;
		DownModeEngaged = false;
		SideModeEngaged = false;
		
		wasClockwise = false;
		wasCounterClockwise = false;

		//Debug.Log ("Joystick OnPointerDown eventData.position(" + eventData.position + ") ZoneCenter("+ZoneCenter+") disallowSwipeForThisTouch("+disallowSwipeForThisTouch+")");
	}
	
	public void OnDrag(PointerEventData eventData) {
		ProcessStick(eventData);
		CheckForModeEngage();
		ConsiderSwipe(eventData.position, true);
	}

	public void OnPointerUp(PointerEventData eventData) {
		ProcessStick(eventData);
		ConsiderSwipe(eventData.position, false);

		Vector2 throwVector = stick.anchoredPosition;
		
		//if(dynamic) bg.anchoredPosition = Vector3.zero;
		
		if(clearHorizontal) {
			throwVector.x = 0f;
		}
		
		if(clearVertical) {
			throwVector.y = 0f;
		}
		
		stick.anchoredPosition = throwVector;
		
		IsPressed = false;
		PressedTime = 0f;
		SwipeRequest = SwipedDirection.sqrMagnitude > 0f;
		bg.gameObject.SetActive(!dynamic || SwipeRequest);
		stick.gameObject.SetActive(!dynamic || SwipeRequest);
		//Debug.Log ("Joystick OnPointerUp StickCenterOnScreen(" + StickCenterOnScreen + ") stick.anchoredPosition("+stick.anchoredPosition+")");
		
		UpModeEngaged = false;
		DownModeEngaged = false;
		SideModeEngaged = false;
	}
#endregion

}

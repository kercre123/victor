using UnityEngine;

/// <summary>
/// very simple touch screen controls for the 3d view of cozmo's game layout instructions camera
/// </summary>
public class TouchCamera : MonoBehaviour {

	Camera cam;

	Vector2?[] oldTouchPositions = {
		null,
		null
	};
	Vector2 oldTouchVector;
	float oldTouchDistance;

	Rect rect;

	void Awake() {
		cam = GetComponent<Camera>();
		rect = new Rect(0, 0, 1, 1);
	}

	int touchCount {
		get {
			if(Input.touchSupported) return Input.touchCount;
			return Input.GetMouseButton(1) ? 2 : Input.GetMouseButton(0) ? 1 : 0;
		}
	}

	Vector2 touch0 {
		get {
			if(Input.touchSupported) return Input.GetTouch(0).position;
			return Input.mousePosition;
		}
	}

	Vector2 touch1 {
		get {
			if(Input.touchSupported) return Input.GetTouch(1).position;
			return (Vector2)Input.mousePosition + Vector2.one;
		}
	}

	void Update() {
		if (touchCount == 0) {
			oldTouchPositions[0] = null;
			oldTouchPositions[1] = null;
			return;
		}

		Vector2 viewPortTouchPos = cam.ScreenToViewportPoint(touch0);
		if(!rect.Contains(viewPortTouchPos)) return;

		if (touchCount == 1) {
			if (oldTouchPositions[0] == null || oldTouchPositions[1] != null) {
				oldTouchPositions[0] = touch0;
				oldTouchPositions[1] = null;
			}
			else {
				Vector2 newTouchPosition = touch0;

				Vector2 delta = (Vector2)newTouchPosition - (Vector2)oldTouchPositions[0];

				float panFactor = Mathf.Abs(delta.x) / GetComponent<Camera>().pixelWidth;
				float panAngle = Mathf.Lerp(0f, 180f, panFactor ) * Mathf.Sign(delta.x);

				float pitchFactor = Mathf.Abs(delta.y) / GetComponent<Camera>().pixelHeight;
				float pitchAngle = Mathf.Lerp(0f, 90f, pitchFactor ) * -Mathf.Sign(delta.y);

				//for now target is origin...this will likely change to a transform
				Vector3 targetToCam = transform.position - Vector3.zero;

				targetToCam = Quaternion.AngleAxis(pitchAngle, transform.right) * targetToCam;

				float angleFromUp = Vector3.Angle(Vector3.up, targetToCam);

				if(angleFromUp > 70f) {
					float overPitch = angleFromUp - 70f;
					targetToCam = Quaternion.AngleAxis(overPitch, transform.right) * targetToCam;
				}
				else if(angleFromUp < 10f) {
					float underPitch = 10f - angleFromUp;
					targetToCam = Quaternion.AngleAxis(-underPitch, transform.right) * targetToCam;
				}

				targetToCam = Quaternion.AngleAxis(panAngle, Vector3.up) * targetToCam;

				//for now target is origin...this will likely change to a transform
				transform.position = targetToCam + Vector3.zero;
				transform.rotation = Quaternion.LookRotation(-targetToCam);
				//transform.position += transform.TransformDirection((Vector3)((oldTouchPositions[0] - newTouchPosition) * camera.orthographicSize / camera.pixelHeight * 2f));
				//Debug.Log("panAngle("+panAngle+") panFactor("+panFactor+") pitchAngle("+pitchAngle+") pitchAngle("+pitchAngle+") delta("+delta+")");
				oldTouchPositions[0] = newTouchPosition;
			}
		}
		else {
			if (oldTouchPositions[1] == null) {
				oldTouchPositions[0] = touch0;
				oldTouchPositions[1] = touch1;
				oldTouchVector = (Vector2)(oldTouchPositions[0] - oldTouchPositions[1]);
				oldTouchDistance = oldTouchVector.magnitude;
			}
			else {
				//Vector2 screen = new Vector2(camera.pixelWidth, camera.pixelHeight);
				
				Vector2[] newTouchPositions = {
					touch0,
					touch1
				};
				Vector2 newTouchVector = newTouchPositions[0] - newTouchPositions[1];
				float newTouchDistance = newTouchVector.magnitude;

				//transform.position += transform.TransformDirection((Vector3)((oldTouchPositions[0] + oldTouchPositions[1] - screen) * camera.orthographicSize / screen.y));
				//transform.localRotation *= Quaternion.Euler(new Vector3(0, 0, Mathf.Asin(Mathf.Clamp((oldTouchVector.y * newTouchVector.x - oldTouchVector.x * newTouchVector.y) / oldTouchDistance / newTouchDistance, -1f, 1f)) / 0.0174532924f));
				GetComponent<Camera>().orthographicSize *= oldTouchDistance / newTouchDistance;
				//transform.position -= transform.TransformDirection((newTouchPositions[0] + newTouchPositions[1] - screen) * camera.orthographicSize / screen.y);

				oldTouchPositions[0] = newTouchPositions[0];
				oldTouchPositions[1] = newTouchPositions[1];
				oldTouchVector = newTouchVector;
				oldTouchDistance = newTouchDistance;
			}
		}
	}
}

using UnityEngine;
using System.Collections;
using UnityEngine.EventSystems;

public class Joystick : MonoBehaviour, IPointerDownHandler, IDragHandler, IPointerUpHandler {
	[SerializeField] RectTransform stick = null;
	[SerializeField] Vector2 deadZone = new Vector2 (0.01f, 0.01f);

    public Vector2 JoystickData {
        get
        {
            Vector2 res = stick.position - startPosition;
            return res / radius;
        }
    }

    public float Horizontal {
        get { 
			float x = JoystickData.x;
			if(Mathf.Abs (x) < deadZone.x) return 0f;
			return x; 
		}
    }

    public float Vertical {
		get { 
			float y = JoystickData.y;
			if(Mathf.Abs (y) < deadZone.y) return 0f;
			return y; 
		}
    }

    Vector3 startPosition;
    float radius;
    Canvas canvas;
	RectTransform rTrans;

    void Awake() {
        canvas = GetComponentInParent<Canvas>();
        startPosition = stick.position;
		rTrans = transform as RectTransform;
        radius = Mathf.Min(rTrans.sizeDelta.x * canvas.transform.localScale.x, rTrans.sizeDelta.y * canvas.transform.localScale.y) * 0.5f;

		//Debug.Log ("Joystick Awake startPosition(" + startPosition + ") stick.position("+stick.position+") stick.anchoredPosition("+stick.anchoredPosition+")");

    }

    public void OnPointerDown(PointerEventData eventData) {
        ProcessStick(eventData);
		//Debug.Log ("Joystick OnPointerDown startPosition(" + startPosition + ")");
    }

    public void OnDrag(PointerEventData eventData) {
        ProcessStick(eventData);
    }

    public void OnPointerUp(PointerEventData eventData) {
        stick.position = startPosition;
		stick.anchoredPosition = Vector3.zero;
		//Debug.Log ("Joystick OnPointerUp startPosition(" + startPosition + ") stick.position("+stick.position+") stick.anchoredPosition("+stick.anchoredPosition+")");
    }

    void ProcessStick(PointerEventData evnt) {
		Vector2 delta = Vector2.zero;

        switch(canvas.renderMode) {
			case RenderMode.ScreenSpaceOverlay:
	            delta = evnt.position - (Vector2)startPosition;
	            delta = Vector3.ClampMagnitude(delta, radius);
	            stick.position = (Vector2)startPosition + delta;
				break;
			case RenderMode.WorldSpace:
			case RenderMode.ScreenSpaceCamera:
				Camera cam = canvas.worldCamera;
				Ray ray = cam.ScreenPointToRay(evnt.position);
				Plane plane = new Plane(transform.forward, transform.position);
				float dist = 0f;
				plane.Raycast(ray, out dist);
				Vector3 point = ray.GetPoint(dist);
				delta = point - startPosition;
				delta = Vector3.ClampMagnitude(delta, radius);
				stick.position = startPosition + (Vector3)delta;
				break;
        }

		//Debug.Log ("Joystick renderMode(" + canvas.renderMode + ") startPosition(" + startPosition + ") delta(" + delta + ") stick.position("+stick.position+") stick.anchoredPosition("+stick.anchoredPosition+")");
    }
}

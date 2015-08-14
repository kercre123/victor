using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class AccelControls : MonoBehaviour {

	[SerializeField] float xMin = 0f;
	[SerializeField] float xMax = 0.5f;

	[SerializeField] float yMin = 0f;
	[SerializeField] float yMax = 0.5f;

	[SerializeField] Text xLabel;
	[SerializeField] Text yLabel;

	float xRange = 1f;
	float yRange = 1f;

	void OnEnable() {
		xRange = xMax - xMin;
		yRange = yMax - yMin;
	}

	float x = 0f;
	int lastXStamp = 0;
	public float Horizontal {
		get {
			if(!SystemInfo.supportsAccelerometer) return 0f;

			if(Time.frameCount == lastXStamp) return x;
			lastXStamp = Time.frameCount;

			float mag = Mathf.Abs( Input.acceleration.x );

			if ( mag < xMin ) {
				x = 0f;
			}
			else if(mag >= xMax) {
				x = Input.acceleration.x > 0f ? 1f : -1f;
			}
			else {
				x = Mathf.Lerp(0f, 1f, (mag - xMin) / xRange);
				x *= Input.acceleration.x > 0f ? 1f : -1f;
			}

			return x; 
		}
	}

	float y = 0f;
	int lastYStamp = 0;
	public float Vertical {
		get { 
			if(!SystemInfo.supportsAccelerometer) return 0f;
			
			if(Time.frameCount == lastYStamp) return y;
			lastYStamp = Time.frameCount;
			
			float mag = Mathf.Abs( Input.acceleration.y );
			
			if ( mag < yMin ) {
				y = 0f;
			}
			else if(mag >= yMax) {
				y = Input.acceleration.y > 0f ? 1f : -1f;
			}
			else {
				y = Mathf.Lerp(0f, 1f, (mag - yMin) / yRange);
				y *= Input.acceleration.y > 0f ? 1f : -1f;
			}
			
			return y; 
		}
	}

	void Update() {
		if(xLabel != null) xLabel.text = "xA(" +Input.acceleration.x+ ") \nxB("+x+")";
		if(yLabel != null) yLabel.text = "yA(" +Input.acceleration.y+ ") \nyB("+y+")";
	}

}

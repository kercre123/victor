using UnityEngine;
using System.Collections;

public class LookAround : MonoBehaviour {
	
	private Vector3 lookAtPointLeft = new Vector3(200f, 20f, 200f);
	private Vector3 lookAtPointRight = new Vector3(100f, 20f, 100f);
	
	private Vector3 lookAtPoint;
	private Vector3 lookAtDestination;
	
	private float speed = 1.0f;
	void Start()
	{
		lookAtPoint = lookAtPointRight;
	}
	
	void Update () {
		if(Vector3.Distance(lookAtPoint, lookAtPointRight) < 1)	
		{
			lookAtDestination = lookAtPointLeft;
		}
		else if(Vector3.Distance(lookAtPoint, lookAtPointLeft) < 1)
		{
			lookAtDestination = lookAtPointRight;
		}
		
		lookAtPoint = Vector3.Lerp(lookAtPoint, lookAtDestination,Time.deltaTime*speed);
		
		camera.transform.LookAt(lookAtPoint);
	}
}

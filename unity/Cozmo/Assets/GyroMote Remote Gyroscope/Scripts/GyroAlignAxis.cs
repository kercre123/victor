using UnityEngine;
using System.Collections;

public class GyroAlignAxis : MonoBehaviour {
	
	private Gyroscope gyroscope;
	private RemoteGyroscope remoteGyroscope;
	public Transform gravity;
	
	// Use this for initialization
	void Start () {
		gyroscope = Input.gyro;
	}
	
	// Update is called once per frame
	void Update () {
		if (SystemInfo.supportsGyroscope) {
			Vector3 position = new Vector3( gyroscope.gravity.x * -1, gyroscope.gravity.z, gyroscope.gravity.y );
			gravity.transform.position = position;
		}
		else {
			if (remoteGyroscope == null)
				remoteGyroscope = GyroMote.gyro();
			
			if (remoteGyroscope) {
				gravity.transform.position = new Vector3(remoteGyroscope.gravity.x * -1, remoteGyroscope.gravity.z, remoteGyroscope.gravity.y);	
			}
		}
		
		transform.LookAt(gravity);
	}
}

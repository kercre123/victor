using UnityEngine;
using System.Collections;

public class RemoteGyroscope : MonoBehaviour {
	
	public Vector3 rotationRate;
	public Vector3 rotationRateUnbiased;
	public Vector3 gravity;
	public Vector3 userAcceleration;
	public Quaternion attitude;
	public float updateInterval;
	
	// Use this for initialization
	void Start () {
		name = "RemoteGyroscope";
	}
	
	void Update () {
		if (!Network.isClient && !Network.isServer)
			Destroy(this.gameObject);
	}
	
	// Update is called once per frame
	public void UpdateGyroscopeViaRPC (Vector3 rR, Vector3 rRu, Vector3 g, Vector3 uA, Quaternion a, float uI) {
		GetComponent<NetworkView>().RPC("UpdateGyroscope", RPCMode.AllBuffered, rR, rRu, g, uA, a, uI);
    }
	
    [RPC]
    void UpdateGyroscope(Vector3 rR, Vector3 rRu, Vector3 g, Vector3 uA, Quaternion a, float uI) {
        rotationRate = rR;
		rotationRateUnbiased = rRu;
		gravity = g;
		userAcceleration = uA;
		attitude = a;
		updateInterval = uI;
    }
	
	void OnGUI() {
		GUI.Box(new Rect(10,10,240,120), "RemoteGyroscope Gyroscope GUI");
		
		GUI.Label(new Rect(20,40,230,110), "rotationRate : " + rotationRate + "\nrotationRateUnbiased : " + rotationRateUnbiased + "\ngravity : " + gravity + "\nuserAcceleration : " + userAcceleration + "\nattitude : " + attitude);
	}
}

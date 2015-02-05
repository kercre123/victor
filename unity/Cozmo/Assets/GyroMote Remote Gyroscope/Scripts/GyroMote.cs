using UnityEngine;
using System.Collections;
using System;

public class GyroMote : MonoBehaviour {
	
	private GyroMoteClient client;
	private GyroMoteServer server;
	private int myType = 0;
	
	// Use this for initialization
	void Start () {
	
		if (SystemInfo.supportsGyroscope) {
				Debug.Log("Gyroscope is supported!");
				StartServer();
			} else {
				Debug.Log ("Gyroscope is unsupported");
				StartClient();
			}
	}
	
	void OnApplicationFocus (bool focus) {
		if (focus) {
			
		}
	}
	
	// Update is called once per frame
	void Update () {
		if (myType == 1)
			server.Update();
	}
	
	void OnDisable() {
		Stop ();	
	}
	
	void OnDestroy() {
		Stop ();
	}
	
	void Stop() {
		Network.Disconnect();	
	}
	
	void OnGUI() {

		if (myType == 1)
			server.renderGUI();
		else if (myType == 2)
			client.renderGUI();
	}
	
	private void StartServer() {
		server = new GyroMoteServer();
		server.position = transform.position;
		myType = 1;
	}
	
	void OnApplicationPause() {
		Network.Disconnect();	
	}
	
	private void StartClient() {
		client = new GyroMoteClient();
		myType = 2;
	}
	
	public static string ConnectionString() {
		return ("GyroMote v" + 1);
	}
	
	void OnPlayerDisconnected (NetworkPlayer player) {
		Network.RemoveRPCs(player, 0);
		Network.DestroyPlayerObjects(player);
	}
	
	void OnPlayerConnected(NetworkPlayer player) {
		Debug.Log ("OnPlayerConnected in GyroMote");
		server.OnPlayerConnected(player);
	}
	
	public static RemoteGyroscope gyro() {
		//Debug.Log ("looking for remote Gyroscope");
		RemoteGyroscope _remoteGyroscope = null;
		
		GameObject[] gameObjects = UnityEngine.Object.FindObjectsOfType(typeof(GameObject)) as GameObject[];
		
		foreach (GameObject gameObject in gameObjects ) {
			
			//Debug.Log (gameObject.name);
			
			if (gameObject.name == "RemoteGyroscope") {
				//Debug.Log("found remote Gyroscope");
				_remoteGyroscope = gameObject.GetComponent("RemoteGyroscope") as RemoteGyroscope;
			}
		   	
		}
		
		return _remoteGyroscope;
	}
}
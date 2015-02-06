using UnityEngine;
using System.Collections;

public class GyroMoteClient {
	
	private string stringIP;
	private int port;
	
	public void renderGUI() {
		
		if (Network.peerType == NetworkPeerType.Disconnected) {
			
			int boxWidth = 240;
			int boxHeight = 120;
			
			int originX = (Screen.width / 2) - (boxWidth / 2);
			int originY = (Screen.height / 2) - (boxHeight / 2);
			
			GUI.Box(new Rect(originX,originY,boxWidth,boxHeight), "Enter Gyroscope Device IP");
			
			stringIP = GUI.TextField(new Rect(originX + 10, originY + 30, boxWidth - 20, 20), stringIP, 15);
			
			if (GUI.Button(new Rect(originX + 10, originY + 60, boxWidth - 20, 20), "Connect")) {
				Connect( stringIP );
			}
		}
	}
	
	public GyroMoteClient () {
		Debug.Log("GyroMote CLIENT created");
		
		stringIP = "192.168.1.190";
	}
	
	~GyroMoteClient () {		
		Debug.Log ("GyroMote Client destroyed");
	}
	
	void Connect (string IP) {
		
		for (port = GyroMoteServer.startPort; port < (GyroMoteServer.startPort + GyroMoteServer.portRange); port++) {
			if( Network.Connect(IP, port, GyroMote.ConnectionString()) == NetworkConnectionError.NoError) {	
				break;
			}
		}
		
		if (Network.peerType == NetworkPeerType.Client || Network.peerType == NetworkPeerType.Connecting) {
			Debug.Log ("Connected to "+ IP + ":" + port);
		} else {
			Debug.Log ("unable to connect to "+ IP + ":" + port);
		}
	}
}

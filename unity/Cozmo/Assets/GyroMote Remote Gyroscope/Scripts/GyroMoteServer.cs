/*
 * Download the GyroMote Server from the App Store. The Server code is not part of this asset!
 * http://www.codingmerc.com/blog/gyromote-unity3d-remote-gyroscope/
 */

using UnityEngine;
using System.Collections;

public class GyroMoteServer {
	
	public static int startPort = 31337;
	public static int portRange = 5;
	
	public Vector3 position;
	
	public void renderGUI() { }
	
	public GyroMoteServer () { }
	
	public void Update() { }
	
	public void OnConnectedToServer () { }
	
	void OnPlayerDisconnected (NetworkPlayer player) { }
	
	public void OnPlayerConnected(NetworkPlayer player) { }
}

using UnityEngine;
using System.Collections;

public class RobotPleaseLookAtFace : MonoBehaviour {

	protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

//	void Update () {
//		FaceFace();
//	}

	public void FaceFace () {
		if(robot != null) {
			
			for(int i=0;i<robot.knownObjects.Count;i++) {
				if(!robot.knownObjects[i].isFace) continue;
				
				Debug.Log("frame("+Time.frameCount+") Update_PLAYING TrackHeadToObject("+robot.knownObjects[i]+") headTrackingObject("+robot.headTrackingObject+")");
				
				robot.TrackHeadToObject(robot.knownObjects[i], true);
				break;
			}
			
		}
	}
}

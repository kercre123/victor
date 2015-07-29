using UnityEngine;
using System.Collections;

/// <summary>
/// made for testing cozmo's ability to look at a human head that he has located
/// </summary>
public class RobotPleaseLookAtFace : MonoBehaviour {

	protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }
	ObservedObject humanHead;

	public void FaceFace () {
		if(robot != null) {
			
			for(int i=0;i<robot.knownObjects.Count;i++) {
				if(!robot.knownObjects[i].isFace) continue;
				
				humanHead = robot.knownObjects[i];

				break;
			}
			
			if(!robot.isBusy && humanHead != null) {
				
				if(!humanHead.MarkersVisible) {
					robot.FaceObject(humanHead, false);
				}
				else if(robot.headTrackingObject == null) {
					robot.TrackToObject(humanHead, false);
				}
			}
		}
	}
}

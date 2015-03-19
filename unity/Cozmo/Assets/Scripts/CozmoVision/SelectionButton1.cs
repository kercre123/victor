using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton1 : SelectionButton
{
	public LineRenderer line;
	public CozmoVision1.SelectionBox box { get; set; }

	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObjects.Clear();

		for(int i=0; i<RobotEngineManager.instance.current.observedObjects.Count; i++) {
			if(RobotEngineManager.instance.current.observedObjects[i].ID != box.ID) continue;
			RobotEngineManager.instance.current.selectedObjects.Add(RobotEngineManager.instance.current.observedObjects[i]);
			RobotEngineManager.instance.TrackHeadToObject( RobotEngineManager.instance.current.observedObjects[i], Intro.CurrentRobotID );
			break;
		}
	}
}

using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton2 : SelectionButton
{
	[System.NonSerialized] public int ID;

	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObjects.Clear();

		for(int i=0; i<RobotEngineManager.instance.current.observedObjects.Count; i++) {
			if(RobotEngineManager.instance.current.observedObjects[i].ID != ID) continue;
			RobotEngineManager.instance.current.selectedObjects.Add(RobotEngineManager.instance.current.observedObjects[i]);
			RobotEngineManager.instance.TrackHeadToObject( RobotEngineManager.instance.current.observedObjects[i], Intro.CurrentRobotID );
			break;
		}

	}
}

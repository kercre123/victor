using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ObservedObjectButton : ObservedObjectBox
{
	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObjects.Clear();
		RobotEngineManager.instance.current.selectedObjects.Add(observedObject);
		RobotEngineManager.instance.current.TrackHeadToObject(observedObject);
	}
}

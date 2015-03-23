using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton2 : SelectionButton
{
	[System.NonSerialized] public ObservedObject observedObject;

	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObjects.Clear();
		RobotEngineManager.instance.current.selectedObjects.Add(observedObject);
		RobotEngineManager.instance.current.TrackHeadToObject(observedObject);
	}
}

using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton2 : SelectionButton
{
	[System.NonSerialized] public int ID;

	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObject = ID;
		RobotEngineManager.instance.TrackHeadToObject( ID, Intro.CurrentRobotID );
	}
}

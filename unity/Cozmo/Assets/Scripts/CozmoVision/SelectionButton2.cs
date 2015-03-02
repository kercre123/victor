using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton2 : SelectionButton
{
	[System.NonSerialized] public uint ID;

	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObject = ID;
	}
}

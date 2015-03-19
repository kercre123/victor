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
		RobotEngineManager.instance.current.selectedObjects.Add(box.observedObject);
		RobotEngineManager.instance.current.TrackHeadToObject(box.observedObject);
	}
}

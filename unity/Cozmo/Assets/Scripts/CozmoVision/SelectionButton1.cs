using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton1 : SelectionButton
{
	public LineRenderer line;
	public CozmoVision1.SelectionBox selectionBox { get; set; }

	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObject = selectionBox.ID;
	}
}

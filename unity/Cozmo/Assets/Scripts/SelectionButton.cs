using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton : MonoBehaviour
{
	public Text text;
	public ImageTest.SelectionBox selectionBox { get; set; }
	
	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObject = selectionBox.ID;
	}
}

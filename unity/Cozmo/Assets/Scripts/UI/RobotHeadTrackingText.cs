using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotHeadTrackingText : MonoBehaviour
{
	[SerializeField] private Text text;

	private int lastHeadTrackingObjectID = -1;

	private void Update()
	{
		if( RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			if( RobotEngineManager.instance.current.headTrackingObjectID != lastHeadTrackingObjectID )
			{
				text.text = "Head tracking object: " + RobotEngineManager.instance.current.headTrackingObjectID;
				lastHeadTrackingObjectID = RobotEngineManager.instance.current.headTrackingObjectID;
			}
		}
	}
}

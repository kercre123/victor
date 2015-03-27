using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotStatusText : MonoBehaviour
{
	[SerializeField] private Text text;

	private Robot.StatusFlag lastStatus;

	private void Update()
	{
		if( RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			if( RobotEngineManager.instance.current.status != lastStatus )
			{
				text.text = "Status: " + RobotEngineManager.instance.current.status;
				lastStatus = RobotEngineManager.instance.current.status;
			}
		}
	}
}

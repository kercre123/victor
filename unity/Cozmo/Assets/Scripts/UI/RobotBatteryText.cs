using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotBatteryText : MonoBehaviour
{
	[SerializeField] private Text text;
	
	private float lastBattery;
	
	private void Update()
	{
		if( RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			if( RobotEngineManager.instance.current.batteryPercent != lastBattery )
			{
				text.text = "Battery: " + RobotEngineManager.instance.current.batteryPercent.ToString( "0.0%" );
				lastBattery = RobotEngineManager.instance.current.batteryPercent;
			}
		}
	}
}

using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class Box
{
	public uint ID { get; private set; }
	public float height { get; private set; }
	public float width { get; private set; }
	public float topLeft_x { get; private set; }
	public float topLeft_y { get; private set; }

	public void UpdateInfo( G2U_RobotObservedObject message )
	{
		ID =  message.objectID;
		height = message.height;
		width = message.width;
		topLeft_x = message.topLeft_x;
		topLeft_y = message.topLeft_y;
	}

	public void RemoveInfo()
	{
		ID =  0;
		height = 0;
		width = 0;
		topLeft_x = 0;
		topLeft_y = 0;
	}
}

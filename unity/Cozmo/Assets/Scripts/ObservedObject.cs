using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class ObservedObject
{
	public int ID { get; private set; }
	public float height { get; private set; }
	public float width { get; private set; }
	public float topLeft_x { get; private set; }
	public float topLeft_y { get; private set; }

	public void UpdateInfo( G2U_RobotObservedObject message )
	{
		ID = message.objectID;
		height = message.img_height;
		width = message.img_width;
		topLeft_x = message.img_topLeft_x;
		topLeft_y = message.img_topLeft_y;
		//message.objectFamily;
		//message.objectType;
	}
}

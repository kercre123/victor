using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision2 : CozmoVision
{
	[SerializeField] protected SelectionButton2[] selectionButtons;

	protected void Update()
	{
		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision2" ) == 1 );

		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			robot = RobotEngineManager.instance.current;

			for( int i = 0; i < actionButtons.Length; ++i )
			{
				// if no object selected or being actioned
				actionButtons[i].button.gameObject.SetActive( ( i == 0 && robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK ) || robot.selectedObject > -1 );

				if( i == 0 )
				{
					if( robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK )
					{
						if( robot.selectedObject > -1 )
						{
							actionButtons[i].text.text = "Stack";
						}
						else
						{
							actionButtons[i].text.text = "Drop";
						}
					}
					else
					{
						actionButtons[i].text.text = "Pick Up";
					}
				}
			}

			for( int i = 0; i < maxObservedObjects; ++i )
			{
				if( observedObjectsCount > i && robot.selectedObject == -1 )
				{
					ObservedObject observedObject = robot.observedObjects[i];

					selectionButtons[i].image.rectTransform.sizeDelta = new Vector2( observedObject.width, observedObject.height );
					selectionButtons[i].image.rectTransform.anchoredPosition = new Vector2( observedObject.topLeft_x, -observedObject.topLeft_y );

					selectionButtons[i].text.text = "Select " + observedObject.ID;
					selectionButtons[i].ID = observedObject.ID;

					selectionButtons[i].image.gameObject.SetActive( true );
				}
				else
				{
					selectionButtons[i].image.gameObject.SetActive( false );
				}
			}
		}
	}
}

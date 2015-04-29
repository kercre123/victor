using UnityEngine;
using System.Collections;

public class GoldRushGameActions : GameActions {

	//public override string TARGET { get { return "Search"; } }
	//public override string PICK_UP { get { return "Pick Up"; } }
	public override string DROP { get { if (GoldRushController.instance.inExtractRange) return "Extract"; else return "Deposit"; } }
	public override string STACK { get { return "Place Scanner"; } }
	//public override string ROLL { get { return "Roll"; } }
	//public override string ALIGN { get { return "Align"; } }
	//public override string CHANGE { get { return "Change"; } }
	//public override string CANCEL { get { return "Cancel"; } }
	
	//protected override string TOP { get { return " TOP"; } }
	//protected override string BOTTOM { get { return " BOTTOM"; } }
	// Use this for initialization

	/*
	protected override void OnEnable ()
	{
		base.OnEnable ();
	}

	public override void OnDisable ()
	{
		base.OnDisable ();
	}
	*/
	
	public override void SetActionButtons( bool isSlider = false ) // 0 is bottom button, 1 is top button, 2 is center button
	{
		if( ActionPanel.instance == null ) return;
		
		ActionPanel.instance.DisableButtons();
		
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null ) return;
		
		robot = RobotEngineManager.instance.current;
		buttons = ActionPanel.instance.actionButtons;
		
		if( robot.isBusy ) return;
		
		if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) )
		{
			if( GoldRushController.instance.state == GameController.GameState.BUILDING )
			{
				if( robot.selectedObjects.Count > 0 && buttons.Length > 1 ) buttons[1].SetMode( ActionButton.Mode.STACK, robot.selectedObjects[0] );
			}

			if( GoldRushController.instance.inDepositRange || GoldRushController.instance.inExtractRange )
			{
				buttons[0].SetMode( ActionButton.Mode.DROP, null );
			}
		}
		else
		{
			if( GoldRushController.instance.state == GameController.GameState.BUILDING )
			{
				if( robot.selectedObjects.Count == 1 && robot.selectedObjects[0].Family == 3 )
				{
					buttons[1].SetMode( ActionButton.Mode.PICK_UP, robot.selectedObjects[0] );
				}
				else
				{
					for( int i = 0; i < robot.selectedObjects.Count && i < 2 && i < buttons.Length; ++i )
					{
						if( robot.selectedObjects[i].Family == 3 )
						{
							buttons[i].SetMode( ActionButton.Mode.PICK_UP, robot.selectedObjects[i], i == 0 ? BOTTOM : TOP );
						}
					}
				}
			}
		}
		
		if( buttons.Length > 2 )
		{
			if( isSlider )
			{
				buttons[2].SetMode( ActionButton.Mode.TARGET, null );
			}
			else if( robot.selectedObjects.Count > 0 )
			{
				buttons[2].SetMode( ActionButton.Mode.CANCEL, null );
			}
		}
	}
	
	public override void Drop( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		ActionButtonClick();

		//extract or deposit
		if (GoldRushController.instance.inExtractRange) 
		{
			GoldRushController.instance.BeginExtracting();
		}
		else if (GoldRushController.instance.inDepositRange)
		{
			GoldRushController.instance.BeginDepositing();
		}
	}

	public override void Stack( bool onRelease, ObservedObject selectedObject )
	{
		if( robot == null || !onRelease ) return;

		ActionButtonClick();

		robot.PickAndPlaceObject( selectedObject );
		GoldRushController.instance.goldCollectingObject = selectedObject;
		Debug.Log ("gold collector id: " + GoldRushController.instance.goldCollectingObject);
	}
}

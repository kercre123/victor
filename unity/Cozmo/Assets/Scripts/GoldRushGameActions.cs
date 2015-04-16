using UnityEngine;
using System.Collections;

public class GoldRushGameActions : GameActions {

	//public override string TARGET { get { return "Search"; } }
	//public override string PICK_UP { get { return "Pick Up"; } }
	public override string DROP { get { if (GoldRushController.instance.inExtractRange) return "Extract"; else return "Deposit"; } }
	//public override string STACK { get { return "Stack"; } }
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

	public override void SetActionButtons() // 0 is bottom button, 1 is top button
	{
		if( ActionPanel.instance == null ) return;
		
		ActionPanel.instance.DisableButtons();
		
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null ) return;
		
		robot = RobotEngineManager.instance.current;
		buttons = ActionPanel.instance.actionButtons;
		
		if( robot.isBusy ) return;
		
		if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) )
		{
			if( robot.selectedObjects.Count > 0 && buttons.Length > 1 ) buttons[1].SetMode( ActionButtonMode.STACK );

			if( GoldRushController.instance.inDepositRange || GoldRushController.instance.inExtractRange )
			{
				buttons[0].SetMode( ActionButtonMode.DROP );
			}
		}
		else
		{
			if( robot.selectedObjects.Count == 1 )
			{
				buttons[1].SetMode( ActionButtonMode.PICK_UP );
			}
			else
			{
				for( int i = 0; i < robot.selectedObjects.Count && i < 2 && i < buttons.Length; ++i )
				{
					buttons[i].SetMode( ActionButtonMode.PICK_UP, i, i == 0 ? BOTTOM : TOP );
				}
			}
		}
		
		if( robot.selectedObjects.Count > 0 && buttons.Length > 2 ) buttons[2].SetMode( ActionButtonMode.CANCEL );
	}

	public override void Drop()
	{
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

	public override void Stack()
	{
		if( RobotEngineManager.instance != null ) RobotEngineManager.instance.current.PickAndPlaceObject( selectedObjectIndex );
		GoldRushController.instance.goldCollectingObject = robot.selectedObjects [selectedObjectIndex];
		Debug.Log ("gold collector id: " + GoldRushController.instance.goldCollectingObject.ID);
	}
}

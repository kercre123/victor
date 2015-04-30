using UnityEngine;
using System.Collections;

public class GoldRushGameActions : GameActions {

	//public override string TARGET { get { return "Search"; } }
	//public override string PICK_UP { get { return "Pick Up"; } }
	public override string DROP { get { if (gameController.inExtractRange) return ActionName( "Extract", base.DROP, base.DROP ); else return ActionName( "Deposit", base.DROP, base.DROP ); } }
	public override string STACK { get { return ActionName( "Place Scanner", base.STACK, base.STACK ); } }
	//public override string ROLL { get { return "Roll"; } }
	//public override string ALIGN { get { return "Align"; } }
	//public override string CHANGE { get { return "Change"; } }
	//public override string CANCEL { get { return "Cancel"; } }
	
	//protected override string TOP { get { return " TOP"; } }
	//protected override string BOTTOM { get { return " BOTTOM"; } }
	// Use this for initialization
	private GoldRushController gameController;

	protected override void Awake()
	{
		base.Awake();

		gameController = GetComponent<GoldRushController>();
	}

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
		if( gameController.state == GameController.GameState.BUILDING )
		{
			base.SetActionButtons( isSlider );
			return;
		}

		if( ActionPanel.instance == null ) return;
		
		ActionPanel.instance.DisableButtons();
		
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null ) return;
		
		robot = RobotEngineManager.instance.current;
		buttons = ActionPanel.instance.actionButtons;
		
		if( robot.isBusy ) return;
		
		if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) )
		{
			if( gameController.state == GameController.GameState.PRE_GAME )
			{
				if( robot.selectedObjects.Count > 0 && buttons.Length > 1 ) buttons[1].SetMode( ActionButton.Mode.STACK, robot.selectedObjects[0] );
			}

			if( gameController.inDepositRange || gameController.inExtractRange )
			{
				buttons[0].SetMode( ActionButton.Mode.DROP, null );
			}
		}
		else
		{
			if( gameController.state == GameController.GameState.PRE_GAME )
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
		if (gameController.inExtractRange) 
		{
			gameController.BeginExtracting();
		}
		else if (gameController.inDepositRange)
		{
			gameController.BeginDepositing();
		}
	}

	public override void Stack( bool onRelease, ObservedObject selectedObject )
	{
		if( robot == null || !onRelease ) return;

		ActionButtonClick();

		robot.PickAndPlaceObject( selectedObject );
		gameController.goldCollectingObject = selectedObject;
		Debug.Log ("gold collector id: " + gameController.goldCollectingObject);
	}
}

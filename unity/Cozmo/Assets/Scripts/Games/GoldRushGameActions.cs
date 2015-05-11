using UnityEngine;
using System.Collections;

public class GoldRushGameActions : GameActions {

	//public override string TARGET { get { return "Search"; } }
	//public override string PICK_UP { get { return "Pick Up"; } }
	public override string DROP { get { return goldController.inExtractRange ? "Extract" : "Deposit"; } }
	//public override string STACK { get { return "Place Scanner"; } }
	//public override string ROLL { get { return "Roll"; } }
	//public override string ALIGN { get { return "Align"; } }
	//public override string CHANGE { get { return "Change"; } }
	//public override string CANCEL { get { return "Cancel"; } }

	[SerializeField] GoldRushController goldController;

	protected override void Awake()
	{
		base.Awake();
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
	
	protected override void _SetActionButtons( bool isSlider ) // 0 is bottom button, 1 is top button, 2 is center button
	{
		if( goldController.state == GameController.GameState.BUILDING )
		{
			base.SetActionButtons( isSlider );
			return;
		}
		
		if( robot.isBusy ) return;

		if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) )
		{
			if( goldController.state == GameController.GameState.PRE_GAME )
			{
				if( robot.selectedObjects.Count > 0 && buttons.Length > 1 ) buttons[1].SetMode( ActionButton.Mode.STACK, robot.selectedObjects[0] );
			}

			if( goldController.inDepositRange || goldController.inExtractRange )
			{
				if( buttons.Length > 2 && isSlider )
				{
					buttons[2].SetMode( ActionButton.Mode.DROP, null, null, true );
				}
				else if( buttons.Length > 0 )
				{
					buttons[0].SetMode( ActionButton.Mode.DROP, null );
				}
			}
		}
		else
		{
			if( goldController.state == GameController.GameState.PRE_GAME )
			{
				if( robot.selectedObjects.Count == 1 && robot.selectedObjects[0].Family == 3 )
				{
					buttons[2].SetMode( ActionButton.Mode.PICK_UP, robot.selectedObjects[0], " Extractor", true );
				}
				else
				{
					for( int i = 0; i < robot.selectedObjects.Count && i < 2 && i < buttons.Length; ++i )
					{
						if( robot.selectedObjects[i].isActive )
						{
							buttons[2].SetMode( ActionButton.Mode.PICK_UP, robot.selectedObjects[i], " Extractor", true );
						}
					}
				}
			}
		}
		
		if( goldController.state == GameController.GameState.PRE_GAME && buttons.Length > 2 )
		{
			if( isSlider )
			{
				//buttons[2].SetMode( ActionButton.Mode.TARGET, null );
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
		if (goldController.inExtractRange) 
		{
			goldController.BeginExtracting();
		}
		else if (goldController.inDepositRange)
		{
			goldController.BeginDepositing();
		}
	}

	public override void Stack( bool onRelease, ObservedObject selectedObject )
	{
		if( robot == null || !onRelease ) return;

		ActionButtonClick();

		robot.PickAndPlaceObject( selectedObject );
		goldController.goldCollectingObject = selectedObject;
		Debug.Log ("gold collector id: " + goldController.goldCollectingObject);
	}
}

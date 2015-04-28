using UnityEngine;
using System.Collections;

public class GameActions : MonoBehaviour
{
	[SerializeField] protected AudioClip actionButtonSound;
	[SerializeField] protected AudioClip cancelButtonSound;
	[SerializeField] protected Sprite[] actionSprites = new Sprite[(int)ActionButtonMode.NUM_MODES];
	
	public virtual string TARGET { get { if( robot != null && robot.targetLockedObject != null && slider != null && slider.Pressed ) return "Object " + robot.targetLockedObject; return "Search"; } }
	public virtual string PICK_UP { get { return "Pick Up"; } }
	public virtual string DROP { get { return "Drop"; } }
	public virtual string STACK { get { return "Stack"; } }
	public virtual string ROLL { get { return "Roll"; } }
	public virtual string ALIGN { get { return "Align"; } }
	public virtual string CHANGE { get { return "Change"; } }
	public virtual string CANCEL { get { return "Cancel"; } }

	protected virtual string TOP { get { return " TOP"; } }
	protected virtual string BOTTOM { get { return " BOTTOM"; } }

	[System.NonSerialized] public int selectedObjectIndex;

	protected Robot robot;
	protected ActionButton[] buttons;
	protected ActionSlider slider;

	public static GameActions instance = null;

	protected virtual void OnEnable()
	{
		instance = this;

		ActionSliderPanel sliderPanel = ActionPanel.instance as ActionSliderPanel;

		if( sliderPanel != null )
		{
			slider = sliderPanel.actionSlider;
		}
		else
		{
			slider = null;
		}
	}

	public virtual void OnDisable()
	{
		if( instance == this ) instance = null;
	}

	public Sprite GetActionSprite( ActionButtonMode mode )
	{
		return actionSprites[(int)mode];
	}

	public virtual void SetActionButtons() // 0 is bottom button, 1 is top button
	{
		if( ActionPanel.instance == null ) return;

		ActionPanel.instance.DisableButtons();
		
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null ) return;
		
		robot = RobotEngineManager.instance.current;
		buttons = ActionPanel.instance.actionButtons;

		if( robot.isBusy ) return;
		
		if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) )
		{
			if( buttons.Length > 1 )
			{
				if(robot.carryingObject >= 0 && robot.carryingObject.Family == 3)
				{
					buttons[1].SetMode( ActionButtonMode.CHANGE );
				}

			}

			bool stack = false;

			if( robot.selectedObjects.Count > 0 )
			{
				float distance = ((Vector2)robot.selectedObjects[0].WorldPosition - (Vector2)robot.WorldPosition).magnitude;
				if(distance <= CozmoUtil.BLOCK_LENGTH_MM * 2f) {
					buttons[0].SetMode( ActionButtonMode.STACK );
					stack = true;
				}
			}

			if(!stack) buttons[0].SetMode( ActionButtonMode.DROP );
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
					buttons[i].SetMode( ActionButtonMode.PICK_UP, true, i, i == 0 ? BOTTOM : TOP );
				}
			}
		}

		if( buttons.Length > 2 )
		{
			if( slider != null )
			{
				buttons[2].SetMode( ActionButtonMode.TARGET, false );
			}
			else if( robot.selectedObjects.Count > 0 )
			{
				buttons[2].SetMode( ActionButtonMode.CANCEL );
			}
		}
	}

	public virtual void ActionButtonClick()
	{
		if( audio != null )
		{
			audio.volume = 1f;
			audio.PlayOneShot( actionButtonSound, 1f );
		}
	}
	
	public virtual void CancelButtonClick()
	{
		if( audio != null )
		{
			audio.volume = 1f;
			audio.PlayOneShot( cancelButtonSound, 1f );
		}
	}

	public virtual void PickUp()
	{
		Debug.Log( "PickUp" );

		if( robot != null ) {
			robot.PickAndPlaceObject( selectedObjectIndex );
			if(CozmoBusyPanel.instance != null)	{

				ObservedObject obj = robot.selectedObjects[selectedObjectIndex];
				if(obj != null) {
					string desc = "Cozmo is attempting to pick-up\n";

					if(obj.Family == 3) {
						desc += "an Active Block.";
					}
					else {
						desc += "a ";

						if(CozmoPalette.instance != null) {
							desc += CozmoPalette.instance.GetNameForObjectType((int)obj.ObjectType) + " ";
						}

						desc += "Block.";
					}


					CozmoBusyPanel.instance.SetDescription(desc);
				}
			}
		}
	}
	
	public virtual void Drop()
	{
		Debug.Log( "Drop" );

		if( robot != null ) {
			robot.PlaceObjectOnGroundHere();
			if(CozmoBusyPanel.instance != null)	{
				
				ObservedObject obj = robot.carryingObject;
				if(obj != null) {
					string desc = "Cozmo is attempting to drop\n";
					
					if(obj.Family == 3) {
						desc += "an Active Block.";
					}
					else {
						desc += "a ";
						
						if(CozmoPalette.instance != null) {
							desc += CozmoPalette.instance.GetNameForObjectType((int)obj.ObjectType) + " ";
						}
						
						desc += "Block.";
					}
					
					CozmoBusyPanel.instance.SetDescription(desc);
				}
			}
		}
	}
	
	public virtual void Stack()
	{
		Debug.Log( "Stack" );

		if( robot != null ) {
			robot.PickAndPlaceObject( selectedObjectIndex );
			if(CozmoBusyPanel.instance != null)	{
				
				ObservedObject obj = robot.carryingObject;
				if(obj != null) {
					string desc = "Cozmo is attempting to stack\n";
					
					if(obj.Family == 3) {
						desc += "an Active Block";
					}
					else {
						desc += "a ";
						
						if(CozmoPalette.instance != null) {
							desc += CozmoPalette.instance.GetNameForObjectType((int)obj.ObjectType) + " ";
						}
						
						desc += "Block";
					}

					ObservedObject target = robot.selectedObjects[selectedObjectIndex];
					if(target != null) {
						
						desc += "\n on top of ";

						if(target.Family == 3) {
							desc += "an Active Block";
						}
						else {
							desc += "a ";
							
							if(CozmoPalette.instance != null) {
								desc += CozmoPalette.instance.GetNameForObjectType((int)target.ObjectType) + " ";
							}
							
							desc += "Block";
						}
					}

					desc += ".";

					CozmoBusyPanel.instance.SetDescription(desc);
				}
			}
		}
	}
	
	public virtual void Roll()
	{
		Debug.Log( "Roll" );
	}
	
	public virtual void Align()
	{
		Debug.Log( "Align" );
	}
	
	public virtual void Change()
	{
		Debug.Log( "Change" );

		if( robot != null && robot.carryingObject != null && robot.carryingObject.Family == 3)
		{
			int typeIndex = (int)robot.carryingObject.activeBlockType + 1;
			if(typeIndex >= (int)ActiveBlockType.NumTypes) typeIndex = 0;
			robot.carryingObject.activeBlockType = (ActiveBlockType)typeIndex;
			robot.carryingObject.SetActiveObjectLEDs( CozmoPalette.instance.GetUIntColorForActiveBlockType( robot.carryingObject.activeBlockType ) );
		}
	}
	
	public virtual void Cancel()
	{
		Debug.Log( "Cancel" );
		
		if( robot != null )
		{
			robot.selectedObjects.Clear();
			robot.SetHeadAngle();
			robot.targetLockedObject = null;
		}
	}

	public virtual void Target()
	{
		bool targetingPropInHand = robot.selectedObjects.Count > 0 && robot.carryingObject != null && 
			robot.selectedObjects.Find(x => x == robot.carryingObject) != null;
		bool alreadyHasTarget = robot.selectedObjects.Count > 0 && robot.targetLockedObject != null && 
			robot.selectedObjects.Find(x => x == robot.targetLockedObject) != null;
		
		if(targetingPropInHand || !alreadyHasTarget) {
			robot.selectedObjects.Clear();
			robot.targetLockedObject = null;
		}
		
		ObservedObject best = robot.targetLockedObject;
		
		if(robot.selectedObjects.Count == 0) {
			//ObservedObject nearest = null;
			ObservedObject mostFacing = null;
			
			float bestDistFromCoz = float.MaxValue;
			float bestAngleFromCoz = float.MaxValue;
			Vector2 forward = robot.Forward;
			
			for(int i=0; i<robot.pertinentObjects.Count; i++) {
				if(robot.carryingObject == robot.pertinentObjects[i]) continue;
				Vector2 atTarget = robot.pertinentObjects[i].WorldPosition - robot.WorldPosition;
				
				float angleFromCoz = Vector2.Angle(forward, atTarget);
				if(angleFromCoz > 90f) continue;
				
				float distFromCoz = atTarget.sqrMagnitude;
				if(distFromCoz < bestDistFromCoz) {
					bestDistFromCoz = distFromCoz;
					//nearest = robot.pertinentObjects[i];
				}
				
				if(angleFromCoz < bestAngleFromCoz) {
					bestAngleFromCoz = angleFromCoz;
					mostFacing = robot.pertinentObjects[i];
				}
			}
			
			best = mostFacing;
			/*if(nearest != null && nearest != best) {
				Debug.Log("AcquireTarget found nearer object than the one closest to center view.");
				float dist1 = (mostFacing.WorldPosition - robot.WorldPosition).sqrMagnitude;
				if(bestDistFromCoz < dist1 * 0.5f) best = nearest;
			}*/
		}
		
		robot.selectedObjects.Clear();
		
		if(best != null) robot.selectedObjects.Add(best);
		
		if(robot.selectedObjects.Count > 0) {
			//find any other objects in a 'stack' with our selected
			for(int i=0; i<robot.pertinentObjects.Count; i++) {
				if(best == robot.pertinentObjects[i])
					continue;
				if(robot.carryingObject == robot.pertinentObjects[i])
					continue;
				
				float dist = Vector2.Distance((Vector2)robot.pertinentObjects[i].WorldPosition, (Vector2)best.WorldPosition);
				if(dist > best.Size.x * 0.5f) {
					//Debug.Log("AcquireTarget rejecting " + robot.pertinentObjects[i].ID +" because it is dist("+dist+") mm from best("+best.ID+") robot.carryingObjectID("+robot.carryingObjectID+")");
					continue;
				}
				
				robot.selectedObjects.Add(robot.pertinentObjects[i]);
			}
			
			//sort selected from ground up
			robot.selectedObjects.Sort(( obj1, obj2 ) => {
				return obj1.WorldPosition.z.CompareTo(obj2.WorldPosition.z);   
			});
			
			if(robot.targetLockedObject == null) robot.targetLockedObject = robot.selectedObjects[0];
		}
		//Debug.Log("frame("+Time.frameCount+") AcquireTarget targets(" + robot.selectedObjects.Count + ") from pertinentObjects("+robot.pertinentObjects.Count+")");
	}
}

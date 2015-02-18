using UnityEngine;
using System.Collections;

public class RandomMoveCharacter : MonoBehaviour {

	private float speed = 15.0f;
	private Vector3 direction;
	CharacterController controller;
	// Use this for initialization
	void Start () {
		controller = GetComponent<CharacterController>();
		genNewDirection();
	}
	
	private void genNewDirection()
	{
		Vector3 randSpeed = new Vector3(Random.Range(-speed, speed), -10.0f, Random.Range(-speed, speed));
		direction = gameObject.transform.TransformDirection(randSpeed*Time.deltaTime);
	}
	
	private bool needResetDirection
	{
		get
		{
			if(    gameObject.transform.position.x < 50 
				|| gameObject.transform.position.x > 250 
				|| gameObject.transform.position.z < 50 
				|| gameObject.transform.position.z > 250)
				return true;
			return false;
		}
	}
	
	// Update is called once per frame
	void Update () {
	
		if(needResetDirection)
			genNewDirection();
		
		controller.Move(direction);
	}
}

using UnityEngine;
using System.Collections;

namespace WellFired
{
	public class MouseMovementLook : MonoBehaviour 
	{
		private float lookX = 0.0f;
		private float lookY = 0.0f;
		
		public float lookSpeed = 40.0f;
		public float lookSmooth = 0.1f;
		
		public float lookXMax = 20.0f;
		public float lookYMax = 20.0f;
		
		// Update is called once per frame
		void Update() 
		{
			lookX += Input.GetAxis("Mouse X") * lookSpeed * Time.deltaTime;
			lookY += Input.GetAxis("Mouse Y") * lookSpeed * Time.deltaTime;
			
			lookX = Mathf.Clamp(lookX, -lookXMax, lookXMax);
			lookY = Mathf.Clamp(lookY, -lookYMax, lookYMax);
			
			Quaternion rotation = Quaternion.Euler(-lookY, lookX, 0.0f);
			transform.localRotation = Quaternion.Lerp(transform.localRotation, rotation, lookSmooth);
		}
	}
}
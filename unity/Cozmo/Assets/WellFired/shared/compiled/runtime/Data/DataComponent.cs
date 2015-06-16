using UnityEngine;

namespace WellFired.Data
{
	public class DataComponent : MonoBehaviour 
	{
		protected DataBaseEntry Data
		{
			get;
			set;
		}
		
		public void InitFromData(DataBaseEntry data)
		{
			Data = data;
		}
	
		protected virtual void Start() 
		{
			if(Data == null)
			{
				Debug.LogError("Component has started without being Initialized", gameObject);
			}
		}
	}
}
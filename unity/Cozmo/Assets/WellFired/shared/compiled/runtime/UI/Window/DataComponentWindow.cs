using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using WellFired.Data;

namespace WellFired.UI
{
	public class WindowWithDataComponent : Window
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
				Debug.LogError("Window has started without being Initialized, you probably should have called OpenWindowWithData on the windowStack", gameObject);
			}
		}

		public override void Ready()
		{
			base.Ready();
		}
	}
}
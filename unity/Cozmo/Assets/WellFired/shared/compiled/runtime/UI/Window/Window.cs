using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace WellFired.UI
{
	public class Window : MonoBehaviour, IWindow
	{
		public WindowStack WindowStack
		{
			get;
			set;
		}
		
		public virtual GameObject FirstSelectedGameObject 
		{
			get;
			set;
		}
		
		public Action OnClose
		{
			get;
			set;
		}

		public void CloseWindow()
		{
			WindowStack.CloseWindow(this);
			GameObject.Destroy(gameObject);
			if(OnClose != default(Action))
				OnClose();
		}

		public virtual void Ready()
		{

		}
	}
}
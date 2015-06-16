using System.Collections;
using System;
using UnityEngine;

namespace WellFired.Initialization
{
	public class ContextInitializer<T> : MonoBehaviour where T : IInitializationContext
	{
		protected T Context
		{
			get;
			set;
		}

		private void Start()
		{
			if(Context == null)
				throw new Exception("Created an ContextInitializer without passing a Context before Start.");

			Ready();
		}
		
		protected virtual void Ready()
		{
			throw new NotImplementedException("Your class must implement the Ready Method");
		}
		
		public void SetContext(T context)
		{
			if(Context != null)
				throw new Exception("Setting a context on the level initializer when we already have one.");
			if(context == null)
				throw new Exception("Cannot pass a null context to the LevelInitialiser");
	
			if(!context.IsContextSetupComplete())
				throw new Exception("The context passed to this oject has not been setup correctly.");
	
			Context = context;
		}
	}
}
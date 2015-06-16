using System.Collections;
using UnityEngine;

namespace WellFired.Initialization
{
	public interface IInitializationContext 
	{
		bool IsContextSetupComplete();
	}
}
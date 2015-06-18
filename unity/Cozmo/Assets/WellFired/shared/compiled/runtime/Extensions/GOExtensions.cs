using UnityEngine;
using System.Collections;

namespace WellFired.Shared
{
	public class GOExtensions
	{
		static public T Construct<T>() where T : Component
		{
			var newGO = new GameObject(typeof(T).ToString());
			newGO.transform.position = Vector3.zero;
			newGO.transform.rotation = Quaternion.identity;
			newGO.AddComponent(typeof(T));
			return newGO.GetComponent(typeof(T)) as T;
		}
	
		static public T ConstructPersistant<T>() where T : Component
		{
			var newGO = Construct<T>();
			GameObject.DontDestroyOnLoad(newGO.gameObject);
			return newGO.GetComponent(typeof(T)) as T;
		}
		
		static public T ConstructFromResource<T>(string name) where T : Component
		{
			var loadedGameObject = Resources.Load(name) as GameObject;
			var instantiatedGameObject = GameObject.Instantiate(loadedGameObject) as GameObject;
			return instantiatedGameObject.GetComponent(typeof(T)) as T;
		}
		
		static public T ConstructFromResourcePersistant<T>(string name) where T : Component
		{
			var loadedGameObject = Resources.Load(name) as GameObject;
			var instantiatedGameObject = GameObject.Instantiate(loadedGameObject) as GameObject;
			GameObject.DontDestroyOnLoad(instantiatedGameObject.gameObject);
			return instantiatedGameObject.GetComponent(typeof(T)) as T;
		}
		
		static public T AddDisabledComponent<T>(GameObject gameObject) where T : MonoBehaviour
		{
			var newComponent = gameObject.AddComponent(typeof(T)) as T;
			newComponent.enabled = false;
			return newComponent;
		}
		
		static public T AddComponentWithData<T>(GameObject gameObject, WellFired.Data.DataBaseEntry data) where T : WellFired.Data.DataComponent
		{
			var newComponent = gameObject.AddComponent(typeof(T)) as T;
			newComponent.InitFromData(data);
			return newComponent;
		}
		
		static public T AddDisabledComponentWithData<T>(GameObject gameObject, WellFired.Data.DataBaseEntry data) where T : WellFired.Data.DataComponent
		{
			var newComponent = gameObject.AddComponent(typeof(T)) as T;
			newComponent.InitFromData(data);
			newComponent.enabled = false;
			return newComponent;
		}
	}
}
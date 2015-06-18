using UnityEngine;
using System;
using System.Collections;
using System.Reflection;

namespace WellFired
{
	[Serializable]
	public class PropertyFieldInfo : ScriptableObject
	{
		[SerializeField]
		public Component Component
		{
			get;
			set;
		}

		public string Name
		{
			get
			{
				if(Property != null)
					return Property.Name;
				if(Field != null)
					return Field.Name;

				return string.Empty;
			}
		}
		
		public string ReadableName
		{
			get
			{
				var name = string.Empty;
				if(Property != null)
					name = Property.Name;
				if(Field != null)
					name = Field.Name;
				
				if(Component.GetType() == typeof(Transform))
				{
					if(name.Contains("rotation"))
						name = name.Replace("rotation", "rotation(Quaternion)");
					if(name.Contains("Rotation"))
						name = name.Replace("Rotation", "Rotation(Quaternion)");
					if(name.Contains("eulerAngles"))
						name = name.Replace("eulerAngles", "rotation(Euler)");
					if(name.Contains("EulerAngles"))
						name = name.Replace("EulerAngles", "Rotation(Euler)");
				}
				
				return name;
			}
		}
		
		[SerializeField]
		private string fieldName;
		private FieldInfo cachedField;
		public FieldInfo Field
		{
			get 
			{
				if(cachedField != null) 
					return cachedField;
				if(!Component || fieldName == null) 
					return null;
				
				Type type = Component.GetType();
				cachedField = WellFired.Shared.PlatformSpecificFactory.ReflectionHelper.GetField(type, fieldName);
				return cachedField;
			}
			set
			{
				if(value != null) 
					propertyName = value.Name;
				else 
					propertyName = null;
				cachedField = value;
			}	
		}
		
		[SerializeField]
		private string propertyName;
		private PropertyInfo cachedProperty;
		public PropertyInfo Property
		{
			get 
			{
				if(cachedProperty != null) 
					return cachedProperty;
				if(!Component || propertyName == null) 
					return null;
				
				Type type = Component.GetType();
				cachedProperty = WellFired.Shared.PlatformSpecificFactory.ReflectionHelper.GetProperty(type, propertyName);
				return cachedProperty;
			}
			set
			{
				if(value != null) 
					propertyName = value.Name;
				else 
					propertyName = null;
				cachedProperty = value;
			}		
		}
	
		private void OnEnable()
		{
			hideFlags = HideFlags.HideAndDontSave;
		}
	}
}
using UnityEngine;
using System;
using System.Collections;
using System.Reflection;

namespace WellFired
{
	public static class PropertyFieldInfoUtility
	{
		public static bool ShouldIgnoreProperty(PropertyInfo property, Component component)
		{
			foreach(Attribute attr in Attribute.GetCustomAttributes(property)) 
			{
				// Check for the Obsolete attribute.
				if (attr.GetType() == typeof(ObsoleteAttribute))
					return true;
			}
			
			if(component.GetType() == typeof(Transform))
			{
				if(property.Name != "position" && property.Name != "rotation" && 
				   property.Name != "eulerAngles" && property.Name != "scale" &&
				   property.Name != "localPosition" && property.Name != "localRotation" && 
				   property.Name != "localEulerAngles" && property.Name != "localScale")
					return true;
			}
			
			if(property.PropertyType == typeof(HideFlags))
				return true;
			
			if(!property.CanWrite)
				return true;
			
			if(!IsValidType(property.PropertyType))
				return true;

			if(property.Name == "useGUILayout")
				return true;
			
			object propertyValue;
			try
			{
				propertyValue = property.GetValue(component, null);
			} 
			catch 
			{
				return true;
			}
			
			if(propertyValue == null)
				return true;
			
			return false;
		}
		
		public static bool shouldIgnoreField(FieldInfo field, Component component)
		{
			if(field.FieldType == typeof(HideFlags))
				return true;
			
			if(!IsValidType(field.FieldType))
				return true;
			
			object fieldValue;
			try
			{
				fieldValue = field.GetValue(component);
			} 
			catch 
			{
				return true;
			}
			
			if(fieldValue == null)
				return true;
			
			return false;
		}

		public static bool IsValidComponent(Type type)
		{
			if(type == typeof(AudioListener) ||
			   type == typeof(Behaviour) ||
			   type == typeof(Tree) ||
			   type == typeof(CanvasRenderer))
				return false;

			return true;
		}
		
		public static bool IsValidType(Type type) 
		{
			if(	(type == typeof(int)) 		|| 
			   (type == typeof(long)) 		|| 
			   (type == typeof(float)) 		|| 
			   (type == typeof(double))		|| 
			   (type == typeof(Vector2)) 	||
			   (type == typeof(Vector3)) 	|| 
			   (type == typeof(Vector4)) 	|| 
			   (type == typeof(Color))		||
			   (type == typeof(bool))		||
			   (type == typeof(Quaternion)))
				return true;
			
			return false;
		}
	}
}
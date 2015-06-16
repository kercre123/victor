using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace WellFired
{
	/// <summary>
	/// Property type information, this enum is all we currently support.
	/// </summary>
	public enum PropertyTypeInfo
	{
		None = -1,
			
		Int,
		Long,
		Float,
		Double,
		Bool,
		Vec2,
		Vec3,
		Vec4,
		Quat,
		Colour,
	};

	public class Modification
	{
		public Modification(USInternalCurve curve, float newTime, float newValue)
		{
			this.curve = curve;
			this.newTime = newTime;
			this.newValue = newValue;
		}

		public USInternalCurve curve;
		public float newTime;
		public float newValue;
	}
	
	/// <summary>
	/// The USPropertyInfo class is a wrapper around the c# PropertyInfo and FieldInfo functionality. 
	/// It is a central location that we can access all information related to a uSequencer Animated Property
	/// We provide easy access functions for common functionality, without having to deal with c# internals.
	/// Scriptable Object doesn't let you use a constructor, so be sure to set up your curve with CreatePropertyInfo
	/// </summary>
	[System.Serializable]
	public class USPropertyInfo : ScriptableObject
	{
		#region Member Variables
		[SerializeField]
		private PropertyTypeInfo propertyType = PropertyTypeInfo.None;
		
		/// <summary>
		/// This is the component that this USPropertyInfo affects. We require a component and a PropertyInfo / Fieldinfo
		/// </summary>
		[SerializeField]
		private Component component;
		
		/// <summary>
		/// This is the component type that this propertyInfo affects, we can use this if we don't have a valid Component. (If we've been serialized)
		/// </summary>
		[SerializeField]
		private string componentType;
		
		/// This is the raw Name of the property or field we want to edit. We need a serializable type.
		public string propertyName;
		public string fieldName;
		
		// We can't serialize these types, so when the user requests them, through the relevant Property, we cache them here.
		private PropertyInfo cachedPropertyInfo;
		private FieldInfo cachedFieldInfo;
		
		/// <summary>
		/// This curves list is our own proprietary format, it's based around AnimationCurve, and still gets
		/// converted to AnimationCurve for Runtime, however, it is easier for the editor to work with.
		/// </summary>
		public List<USInternalCurve> curves = new List<USInternalCurve>();
		
		/// Curves0 - Curves4 used to be used, when we didn't have our own intermediatary format (USInternalCurve),
		/// don't use these.
		[Obsolete("curves0 - curves 4 is now Obsolete, please use the curves list, instead.")]
		public AnimationCurve curves0 = null;
		[Obsolete("curves0 - curves 4 is now Obsolete, please use the curves list, instead.")]
		public AnimationCurve curves1 = null;
		[Obsolete("curves0 - curves 4 is now Obsolete, please use the curves list, instead.")]
		public AnimationCurve curves2 = null;
		[Obsolete("curves0 - curves 4 is now Obsolete, please use the curves list, instead.")]
		public AnimationCurve curves3 = null;
		
		// We keep tmp variables in the class, otherwise we'd be continually creating these whilst sampling the curve.
		private bool tmpBool			= false;
		private Keyframe tmpKeyframe 	= new Keyframe();
		private Vector2 tmpVector2 		= Vector2.zero;
		private Vector3 tmpVector3 		= Vector3.zero;
		private Vector4 tmpVector4 		= Vector4.zero;
		private Quaternion tmpQuat 		= Quaternion.identity;
		private Color tmpColour 		= Color.black;
		
		[SerializeField] private int 		baseInt			= 0;
		[SerializeField] private long		baseLong		= 0;
		[SerializeField] private float 		baseFloat		= 0.0f;
		[SerializeField] private double		baseDouble		= 0.0f;
		[SerializeField] private bool 		baseBool		= false;
		[SerializeField] private Vector2 	baseVector2 	= Vector2.zero;
		[SerializeField] private Vector3 	baseVector3 	= Vector3.zero;
		[SerializeField] private Vector4 	baseVector4 	= Vector4.zero;
		[SerializeField] private Quaternion baseQuat 		= Quaternion.identity;
		[SerializeField] private Color 		baseColour 		= Color.black;
	
		private float previousTime 		= -1.0f;
		#endregion
		
		#region Properties
		/// <summary>
		/// If we have cached info, this is returned, otherwise, it is retrieved, and cached, then returned.
		/// </summary>
		public PropertyInfo propertyInfo
		{
			get 
			{
				if(cachedPropertyInfo != null) 
					return cachedPropertyInfo;
				if(!component || propertyName == null) 
					return null;
					
				Type type = component.GetType();
				cachedPropertyInfo = WellFired.Shared.PlatformSpecificFactory.ReflectionHelper.GetProperty(type, propertyName);
				return cachedPropertyInfo;
			}
			set
			{
				if(value != null) 
					propertyName = value.Name;
				else 
					propertyName = null;
				
				cachedPropertyInfo = value;
			}			
		}	
		
		/// <summary>
		/// If we have cached info, this is returned, otherwise, it is retrieved, and cached, then returned.
		/// </summary>
		public FieldInfo fieldInfo
		{
			get 
			{
				if(cachedFieldInfo != null) 
					return cachedFieldInfo;
				if(!component || fieldName == null) 
					return null;
					
				Type type = component.GetType();
				cachedFieldInfo = WellFired.Shared.PlatformSpecificFactory.ReflectionHelper.GetField(type, fieldName);
				return cachedFieldInfo;
			}
			set
			{
				if(value != null) 
					fieldName = value.Name;
				else 
					fieldName = null;
				
				cachedFieldInfo = value;
			}			
		}
		
		public Component Component
		{
			get { return component; }
			set 
			{	
				component = value;
				if(component != null)
					ComponentType = component.GetType().ToString();
			}
		}
		
		public string ComponentType
		{
			get
			{
				if(componentType == "" && component)
					componentType = component.GetType().ToString();
				
				return componentType;
			}
			set 
			{
				componentType = USRuntimeUtility.ConvertToSerializableName(value);
			}
		}
		
		public string PropertyName
		{
			get
			{
				if(fieldInfo != null)
					return fieldInfo.Name;
				if(propertyInfo != null)
					return propertyInfo.Name;
				throw new Exception("No Property");
			}
			private set { ; }
		}
		
		public PropertyTypeInfo PropertyType
		{
			get
			{
				return propertyType;
			}
		}

		public string InternalName 
		{
			get;
			set;
		}
		#endregion
	
		public void CreatePropertyInfo(PropertyTypeInfo createdPropertyType)
		{
			if(propertyType != PropertyTypeInfo.None)
			{
				Debug.LogError("We are trying to CreatePropertyInfo, but it will be an overwrite!");
				return;
			}
			
			propertyType = createdPropertyType;
			
			CreatePropertyCurves();
		}
		
		public void SetValue(float time)
		{
			if(curves.Count == 0)
				return;
			
			if(propertyType == PropertyTypeInfo.None)
			{
				Debug.LogError("Atempting to Set A Value, before calling CreatePropertyInfo");
				return;
			}
			
			float earliestTime = Mathf.Infinity;
			float latestTime = 0.0f;
	
			foreach(USInternalCurve curve in curves)
			{
				if(earliestTime > curve.FirstKeyframeTime)
					earliestTime = curve.FirstKeyframeTime;
	
				if(latestTime < curve.LastKeyframeTime)
					latestTime = curve.LastKeyframeTime;
			}
	
			bool wasPreviouslyInCurveArea = false;
			bool isCurrentlyInCurveArea = false;
			bool didPlayPassCurveArea = false;
			bool didPlayPassCurveAreaInReverse = false;
			bool isUseCurrentValue = false;
	
			if(previousTime >= earliestTime && previousTime <= latestTime)
				wasPreviouslyInCurveArea = true;
			if(time >= earliestTime && time <= latestTime)
				isCurrentlyInCurveArea = true;
			if((previousTime < earliestTime && time > latestTime) || (previousTime > latestTime && time < earliestTime))
				didPlayPassCurveArea = true;
			if(didPlayPassCurveArea && previousTime > time)
				didPlayPassCurveAreaInReverse = true;

			// This is the correct place to fix up our initial keyframes
			for(int curveIndex = 0; curveIndex < curves.Count; curveIndex++)
			{
				if(!curves[curveIndex].UseCurrentValue || curves[curveIndex] == null)
					continue;

				isUseCurrentValue = true;
				float firstKeyframeTime = curves[curveIndex].FirstKeyframeTime;
				bool isInitialKeyframe = previousTime < firstKeyframeTime && time >= firstKeyframeTime;
				if(isInitialKeyframe)
					BuildForCurrentValue(curveIndex);
			}
	
			previousTime = time;
	
			// if we are not within the curves min and max, exit.
			bool isValidToProcess = false;
	
			if(didPlayPassCurveArea)
				isValidToProcess = true;
			if(isCurrentlyInCurveArea)
				isValidToProcess = true;
			if(!isCurrentlyInCurveArea && wasPreviouslyInCurveArea)
				isValidToProcess = true;
			if(didPlayPassCurveAreaInReverse && isUseCurrentValue)
				isValidToProcess = false;

			// We're not inside any curve's time frames, bail. (Added bonus, performance increase!)
			if(!isValidToProcess)
				return;

			object setValue = null;
			
			// We will convert this SetTime function into one of our internal property specific functions,
			// this is to abstract the curve implementation and data away from the user.
			switch(propertyType)
			{
			case PropertyTypeInfo.Int:
				setValue = (int)curves[0].Evaluate(time);
				break;
			case PropertyTypeInfo.Long:
				setValue = curves[0].Evaluate(time);
				break;
			case PropertyTypeInfo.Float:
				setValue = curves[0].Evaluate(time);
				break;
			case PropertyTypeInfo.Double:
				setValue = curves[0].Evaluate(time);
				break;
			case PropertyTypeInfo.Bool:
				bool bestValue = false;
				foreach(USInternalKeyframe keyframe in curves[0].Keys)
				{
					if(keyframe.Time > time)
						break;
					
					if(keyframe.Value >= 1.0f)
						bestValue = true;
					else if(keyframe.Value <= 0.0f)
						bestValue = false;
				}
				
				if(curves[0].Evaluate(time) >= 1.0f)
					tmpBool = true;
				else if(curves[0].Evaluate(time) <= 0.0f)
					tmpBool = false;
				else
					tmpBool = bestValue;
	
				setValue = tmpBool;
				break;
			case PropertyTypeInfo.Vec2:
				tmpVector2.x = curves[0].Evaluate(time);
				tmpVector2.y = curves[1].Evaluate(time);
				setValue = tmpVector2;
				break;
			case PropertyTypeInfo.Vec3:
				tmpVector3.x = curves[0].Evaluate(time);
				tmpVector3.y = curves[1].Evaluate(time);
				tmpVector3.z = curves[2].Evaluate(time);
				setValue = tmpVector3;
				break;
			case PropertyTypeInfo.Vec4:
				tmpVector4.x = curves[0].Evaluate(time);
				tmpVector4.y = curves[1].Evaluate(time);
				tmpVector4.z = curves[2].Evaluate(time);
				tmpVector4.w = curves[3].Evaluate(time);
				setValue = tmpVector4;
				break;
			case PropertyTypeInfo.Quat:
				tmpQuat.x = curves[0].Evaluate(time);
				tmpQuat.y = curves[1].Evaluate(time);
				tmpQuat.z = curves[2].Evaluate(time);
				tmpQuat.w = curves[3].Evaluate(time);
				setValue = tmpQuat;
				break;
			case PropertyTypeInfo.Colour:
				tmpColour.r = curves[0].Evaluate(time);
				tmpColour.g = curves[1].Evaluate(time);
				tmpColour.b = curves[2].Evaluate(time);
				tmpColour.a = curves[3].Evaluate(time);
				setValue = tmpColour;
				break;
			}
			
			if(setValue == null)
				return;
			
			if(component == null)
				return;
			
			// Great, we sampled the curves, set some values.
			if(fieldInfo != null)
				fieldInfo.SetValue(component, setValue);
			if(propertyInfo != null)
				propertyInfo.SetValue(component, setValue, null);
		}
		
		public void AddKeyframe(List<USInternalCurve> settingCurves, object ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			if(settingCurves.Count == 0)
				return;
			
			if(propertyType == PropertyTypeInfo.None)
			{
				Debug.LogError("Atempting to Add A Keyframe, before calling CreatePropertyInfo");
				return;
			}
			
			if(GetMappedType(ourValue.GetType()) != propertyType)
			{
				Debug.LogError("Trying to Add a Keyframe of the wrong type");
				return;
			}
			
			switch(GetMappedType(ourValue.GetType()))
			{
			case PropertyTypeInfo.Int:
				AddKeyframe((int)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Long:
				AddKeyframe((long)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Float:
				AddKeyframe((float)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Double:
				AddKeyframe((double)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Bool:
				AddKeyframe((bool)(ourValue), time, tangentMode);
				break;
			case PropertyTypeInfo.Vec2:
				{
					Vector2 newVector2 = (Vector2)ourValue;
					tmpKeyframe.time = time;
						
					if(settingCurves.Contains(curves[0]))
					{
						tmpKeyframe.value = newVector2.x;
						AddKeyframe(curves[0], tmpKeyframe, tangentMode); 
					}
					if(settingCurves.Contains(curves[1]))
					{
						tmpKeyframe.value = newVector2.y;
						AddKeyframe(curves[1], tmpKeyframe, tangentMode); 
					}
				}
				break;
			case PropertyTypeInfo.Vec3:
				{
					Vector3 newVector3 = (Vector3)ourValue;
					tmpKeyframe.time = time;
				
					if(settingCurves.Contains(curves[0]))
					{
						tmpKeyframe.value = newVector3.x;
						AddKeyframe(curves[0], tmpKeyframe, tangentMode); 
					}
					if(settingCurves.Contains(curves[1]))
					{
						tmpKeyframe.value = newVector3.y;
						AddKeyframe(curves[1], tmpKeyframe, tangentMode); 
					} 
					if(settingCurves.Contains(curves[2]))
					{
						tmpKeyframe.value = newVector3.z;
						AddKeyframe(curves[2], tmpKeyframe, tangentMode); 
					}
				}
				break;
			case PropertyTypeInfo.Vec4:
				{
					Vector4 newVector4 = (Vector4)ourValue;
					tmpKeyframe.time = time;
				
					if(settingCurves.Contains(curves[0]))
					{
						tmpKeyframe.value = newVector4.x;
						AddKeyframe(curves[0], tmpKeyframe, tangentMode); 
					}
					if(settingCurves.Contains(curves[1]))
					{
						tmpKeyframe.value = newVector4.y;
						AddKeyframe(curves[1], tmpKeyframe, tangentMode); 
					}
					if(settingCurves.Contains(curves[2]))
					{
						tmpKeyframe.value = newVector4.z;
						AddKeyframe(curves[2], tmpKeyframe, tangentMode); 	
					}
					if(settingCurves.Contains(curves[3]))
					{
						tmpKeyframe.value = newVector4.w;
						AddKeyframe(curves[3], tmpKeyframe, tangentMode);
					}
				}
				break;
			case PropertyTypeInfo.Quat:
				{
					Quaternion newQuat = (Quaternion)ourValue;
					tmpKeyframe.time = time;
				
					if(settingCurves.Contains(curves[0]))
					{
						tmpKeyframe.value = newQuat.x;
						AddKeyframe(curves[0], tmpKeyframe, tangentMode); 
					}
					if(settingCurves.Contains(curves[1]))
					{
						tmpKeyframe.value = newQuat.y;
						AddKeyframe(curves[1], tmpKeyframe, tangentMode); 
					}
					if(settingCurves.Contains(curves[2]))
					{
						tmpKeyframe.value = newQuat.z;
						AddKeyframe(curves[2], tmpKeyframe, tangentMode);
					}
					if(settingCurves.Contains(curves[3]))
					{
						tmpKeyframe.value = newQuat.w;
						AddKeyframe(curves[3], tmpKeyframe, tangentMode); 
					}
				}
				break;
			case PropertyTypeInfo.Colour:
				{
					Color newColour = (Color)ourValue;
					tmpKeyframe.time = time;
				
					if(settingCurves.Contains(curves[0]))
					{
						tmpKeyframe.value = newColour.r;
						AddKeyframe(curves[0], tmpKeyframe, tangentMode); 
					}
					if(settingCurves.Contains(curves[1]))
					{
						tmpKeyframe.value = newColour.g;
						AddKeyframe(curves[1], tmpKeyframe, tangentMode); 
					}
					if(settingCurves.Contains(curves[2]))
					{
						tmpKeyframe.value = newColour.b;
						AddKeyframe(curves[2], tmpKeyframe, tangentMode); 
					}
					if(settingCurves.Contains(curves[3]))
					{
						tmpKeyframe.value = newColour.a;
						AddKeyframe(curves[3], tmpKeyframe, tangentMode); 
					}
				}
				break;
			}
		}	
	
		public void AddKeyframe(float time, CurveAutoTangentModes tangentMode)
		{
			object propertyValue = null;
			if(fieldInfo != null)
				propertyValue = fieldInfo.GetValue(component);
			if(propertyInfo != null)
				propertyValue = propertyInfo.GetValue(component, null);
	
			AddKeyframe(propertyValue, time, tangentMode);
		}
		
		// We don't want the user to care about the type of curve, therefore we just have one function exposed.
		// The user can call this with some data, and we will add the keyframe to all needed curves.
		// This function is safe, and if incorrect values or data types are passed, it will complain and not continue.
		public void AddKeyframe(object ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			if(propertyType == PropertyTypeInfo.None)
			{
				Debug.LogError("Atempting to Add A Keyframe, before calling CreatePropertyInfo");
				return;
			}
			
			if(GetMappedType(ourValue.GetType()) != propertyType)
			{
				Debug.LogError("Trying to Add a Keyframe of the wrong type");
				return;
			}
			
			switch(GetMappedType(ourValue.GetType()))
			{
			case PropertyTypeInfo.Int:
				AddKeyframe((int)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Long:
				AddKeyframe((long)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Float:
				AddKeyframe((float)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Double:
				AddKeyframe((double)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Bool:
				AddKeyframe((bool)(ourValue), time, tangentMode);
				break;
			case PropertyTypeInfo.Vec2:
				AddKeyframe((Vector2)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Vec3:
				AddKeyframe((Vector3)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Vec4:
				AddKeyframe((Vector4)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Quat:
				AddKeyframe((Quaternion)ourValue, time, tangentMode);
				break;
			case PropertyTypeInfo.Colour:
				AddKeyframe((Color)ourValue, time, tangentMode);
				break;
			}
		}
		
		private void AddKeyframe(USInternalCurve curve, Keyframe keyframe, CurveAutoTangentModes tangentMode)
		{
			USInternalKeyframe internalKeyframe = curve.AddKeyframe(keyframe.time, keyframe.value);
		
			if(propertyType == PropertyTypeInfo.Bool)
			{
				tangentMode = CurveAutoTangentModes.None;
				internalKeyframe.RightTangentConstant();
				internalKeyframe.LeftTangentConstant();
			}
			
			if(tangentMode == CurveAutoTangentModes.Smooth)
				internalKeyframe.Smooth();
			else if(tangentMode == CurveAutoTangentModes.Flatten)
				internalKeyframe.Flatten();
			else if(tangentMode == CurveAutoTangentModes.RightLinear)
				internalKeyframe.RightTangentLinear();
			else if(tangentMode == CurveAutoTangentModes.LeftLinear)
				internalKeyframe.LeftTangentLinear();
			else if(tangentMode == CurveAutoTangentModes.BothLinear)
				internalKeyframe.BothTangentLinear();

			SetValue(keyframe.time);
		}
		
		private void AddKeyframe(int ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			tmpKeyframe.time = time;
			tmpKeyframe.value = (float)ourValue;
			AddKeyframe(curves[0], tmpKeyframe, tangentMode);
		}
		
		private void AddKeyframe(long ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			tmpKeyframe.time = time;
			tmpKeyframe.value = (float)ourValue;
			AddKeyframe(curves[0], tmpKeyframe, tangentMode);
		}
		
		private void AddKeyframe(float ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue;
			AddKeyframe(curves[0], tmpKeyframe, tangentMode);
		}
		
		private void AddKeyframe(double ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			tmpKeyframe.time = time;
			tmpKeyframe.value = (float)ourValue;
			AddKeyframe(curves[0], tmpKeyframe, tangentMode);
		}
		
		private void AddKeyframe(bool ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			float result = 0.0f;
			
			if(ourValue == true)
				result = 1.0f;
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = result;
			AddKeyframe(curves[0], tmpKeyframe, CurveAutoTangentModes.None);
		}
		
		private void AddKeyframe(Vector2 ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.x;
			AddKeyframe(curves[0], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.y;
			AddKeyframe(curves[1], tmpKeyframe, tangentMode);
		}
		
		private void AddKeyframe(Vector3 ourValue, float time, CurveAutoTangentModes tangentMode)
		{	
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.x;
			AddKeyframe(curves[0], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.y;
			AddKeyframe(curves[1], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.z;
			AddKeyframe(curves[2], tmpKeyframe, tangentMode);
		}
		
		private void AddKeyframe(Vector4 ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.x;
			AddKeyframe(curves[0], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.y;
			AddKeyframe(curves[1], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.z;
			AddKeyframe(curves[2], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.w;
			AddKeyframe(curves[3], tmpKeyframe, tangentMode);
		}
		
		private void AddKeyframe(Quaternion ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.x;
			AddKeyframe(curves[0], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.y;
			AddKeyframe(curves[1], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.z;
			AddKeyframe(curves[2], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.w;
			AddKeyframe(curves[3], tmpKeyframe, tangentMode);
		}
		
		private void AddKeyframe(Color ourValue, float time, CurveAutoTangentModes tangentMode)
		{
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.r;
			AddKeyframe(curves[0], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.g;
			AddKeyframe(curves[1], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.b;
			AddKeyframe(curves[2], tmpKeyframe, tangentMode);
			
			tmpKeyframe.time = time;
			tmpKeyframe.value = ourValue.a;
			AddKeyframe(curves[3], tmpKeyframe, tangentMode);
		}
		
		// Here we will initialise our curves, creating an AnimationCurve for every USInternalCurve
		private void CreatePropertyCurves()
		{	
			switch(propertyType)
			{
			case PropertyTypeInfo.Int:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				break;
			case PropertyTypeInfo.Long:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				break;
			case PropertyTypeInfo.Float:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				break;
			case PropertyTypeInfo.Double:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				break;
			case PropertyTypeInfo.Bool:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				break;
			case PropertyTypeInfo.Vec2:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				curves[1].UnityAnimationCurve = new AnimationCurve();
				break;
			case PropertyTypeInfo.Vec3:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				curves[1].UnityAnimationCurve = new AnimationCurve();
				curves[2].UnityAnimationCurve = new AnimationCurve();
				break;
			case PropertyTypeInfo.Vec4:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				curves[1].UnityAnimationCurve = new AnimationCurve();
				curves[2].UnityAnimationCurve = new AnimationCurve();
				curves[3].UnityAnimationCurve = new AnimationCurve();
				break;
			case PropertyTypeInfo.Quat:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				curves[1].UnityAnimationCurve = new AnimationCurve();
				curves[2].UnityAnimationCurve = new AnimationCurve();
				curves[3].UnityAnimationCurve = new AnimationCurve();
				break;
			case PropertyTypeInfo.Colour:
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				curves.Add(ScriptableObject.CreateInstance<USInternalCurve>());
				
				curves[0].UnityAnimationCurve = new AnimationCurve();
				curves[1].UnityAnimationCurve = new AnimationCurve();
				curves[2].UnityAnimationCurve = new AnimationCurve();
				curves[3].UnityAnimationCurve = new AnimationCurve();
				break;
			}
		}
		
		public object GetValueForTime(float time)
		{
			object getValue = null;
			
			switch(propertyType)
			{
			case PropertyTypeInfo.Int:
				getValue = curves[0].Evaluate(time);
				break;
			case PropertyTypeInfo.Long:
				getValue = curves[0].Evaluate(time);
				break;
			case PropertyTypeInfo.Float:
				getValue = curves[0].Evaluate(time);
				break;
			case PropertyTypeInfo.Double:
				getValue = curves[0].Evaluate(time);
				break;
			case PropertyTypeInfo.Bool:
				bool bestValue = false;
				foreach(USInternalKeyframe keyframe in curves[0].Keys)
				{
					if(keyframe.Time > time)
						break;
					
					if(keyframe.Value >= 1.0f)
						bestValue = true;
					else if(keyframe.Value <= 0.0f)
						bestValue = false;
				}
				
				if(curves[0].Evaluate(time) >= 1.0f)
					tmpBool = true;
				else if(curves[0].Evaluate(time) <= 0.0f)
					tmpBool = false;
				else
					tmpBool = bestValue;
				
				getValue = tmpBool;
				break;
			case PropertyTypeInfo.Vec2:
				tmpVector2.x = curves[0].Evaluate(time);
				tmpVector2.y = curves[1].Evaluate(time);
				getValue = tmpVector2;
				break;
			case PropertyTypeInfo.Vec3:
				tmpVector3.x = curves[0].Evaluate(time);
				tmpVector3.y = curves[1].Evaluate(time);
				tmpVector3.z = curves[2].Evaluate(time);
				getValue = tmpVector3;
				break;
			case PropertyTypeInfo.Vec4:
				tmpVector4.x = curves[0].Evaluate(time);
				tmpVector4.y = curves[1].Evaluate(time);
				tmpVector4.z = curves[2].Evaluate(time);
				tmpVector4.w = curves[3].Evaluate(time);
				getValue = tmpVector4;
				break;
			case PropertyTypeInfo.Quat:
				tmpQuat.x = curves[0].Evaluate(time);
				tmpQuat.y = curves[1].Evaluate(time);
				tmpQuat.z = curves[2].Evaluate(time);
				tmpQuat.w = curves[3].Evaluate(time);
				getValue = tmpQuat;
				break;
			case PropertyTypeInfo.Colour:
				tmpColour.r = curves[0].Evaluate(time);
				tmpColour.g = curves[1].Evaluate(time);
				tmpColour.b = curves[2].Evaluate(time);
				tmpColour.a = curves[3].Evaluate(time);
				getValue = tmpColour;
				break;
			}
			
			if(getValue == null)
				return null;
			
			return getValue;
		}

		/// <summary>
		/// This will return you a list of all curves that have been modified at a given time. You could use this in conjunction with HasPropertyBeenModified.
		/// </summary>
		/// <returns>The modified curves at time.</returns>
		/// <param name="runningTime">Running time.</param>
		public List<Modification> GetModifiedCurvesAtTime(float runningTime)
		{
			var modifiedCurves = new List<Modification>();

			// The object has probably been deleted from the hierarchy.
			if(!component)
				return modifiedCurves;
			
			// if we are not within the curves min and max, exit.
			bool isValidToProcess = false;
			foreach(USInternalCurve curve in curves)
			{
				if(runningTime >= curve.FirstKeyframeTime && runningTime <= curve.LastKeyframeTime)
					isValidToProcess = true;
			}
			
			// We're not inside any curve's time frames, bail.
			if(!isValidToProcess)
				return modifiedCurves;
			
			object propertyValue = null;
			if(fieldInfo != null)
				propertyValue = fieldInfo.GetValue(component);
			if(propertyInfo != null)
				propertyValue = propertyInfo.GetValue(component, null);

			switch(propertyType)
			{
			case PropertyTypeInfo.Int:

				var existingInt = (int)curves[0].Evaluate(runningTime);
				if((int)existingInt != (int)propertyValue)
					modifiedCurves.Add(new Modification(curves[0], runningTime, (float)propertyValue));

				break;

			case PropertyTypeInfo.Long:

				var existingLong = (long)curves[0].Evaluate(runningTime);
				if(existingLong != (long)propertyValue)
					modifiedCurves.Add(new Modification(curves[0], runningTime, (float)propertyValue));
				
				break;

			case PropertyTypeInfo.Float:

				var existingFloat = (float)curves[0].Evaluate(runningTime);
				if(!Mathf.Approximately(existingFloat, (float)propertyValue))
					modifiedCurves.Add(new Modification(curves[0], runningTime, (float)propertyValue));
				
				break;

			case PropertyTypeInfo.Double:

				var existingDouble = (float)curves[0].Evaluate(runningTime);
				if(!Mathf.Approximately(existingDouble, (float)propertyValue))
					modifiedCurves.Add(new Modification(curves[0], runningTime, (float)propertyValue));
				
				break;

			case PropertyTypeInfo.Bool:

				var existingBool = false;
				var curveValue = (float)curves[0].Evaluate(runningTime);
				
				if(curveValue >= 1.0f)
					existingBool = true;
				else
					existingBool = false;

				if(existingBool != (bool)propertyValue)
					modifiedCurves.Add(new Modification(curves[0], runningTime, (float)propertyValue));
				
				break;

			case PropertyTypeInfo.Vec2:

				tmpVector2 = (Vector2)propertyValue;
				var existingVec2X = (float)curves[0].Evaluate(runningTime);
				if(!Mathf.Approximately(existingVec2X, tmpVector2.x))
					modifiedCurves.Add(new Modification(curves[0], runningTime, tmpVector2.x));

				var existingVec2Y = (float)curves[1].Evaluate(runningTime);
				if(!Mathf.Approximately(existingVec2Y, tmpVector2.y))
					modifiedCurves.Add(new Modification(curves[1], runningTime, tmpVector2.y));
				
				break;

			case PropertyTypeInfo.Vec3:
				
				tmpVector3 = (Vector3)propertyValue;
				var existingVec3X = (float)curves[0].Evaluate(runningTime);
				if(!Mathf.Approximately(existingVec3X, tmpVector3.x))
				   modifiedCurves.Add(new Modification(curves[0], runningTime, tmpVector3.x));

				var existingVec3Y = (float)curves[1].Evaluate(runningTime);
				if(!Mathf.Approximately(existingVec3Y, tmpVector3.y))
					modifiedCurves.Add(new Modification(curves[1], runningTime, tmpVector3.y));

				var existingVec3Z = (float)curves[2].Evaluate(runningTime);
				if(!Mathf.Approximately(existingVec3Z, tmpVector3.z))
					modifiedCurves.Add(new Modification(curves[2], runningTime, tmpVector3.z));
				
				break;

			case PropertyTypeInfo.Vec4:
				
				tmpVector4 = (Vector4)propertyValue;
				var existingVec4X = (float)curves[0].Evaluate(runningTime);
				if(!Mathf.Approximately(existingVec4X, tmpVector4.x))
					modifiedCurves.Add(new Modification(curves[0], runningTime, tmpVector4.x));
				
				var existingVec4Y = (float)curves[1].Evaluate(runningTime);
				if(!Mathf.Approximately(existingVec4Y, tmpVector4.y))
					modifiedCurves.Add(new Modification(curves[1], runningTime, tmpVector4.y));
				
				var existingVec4Z = (float)curves[2].Evaluate(runningTime);
				if(!Mathf.Approximately(existingVec4Z, tmpVector4.z))
					modifiedCurves.Add(new Modification(curves[2], runningTime, tmpVector4.z));
				
				var existingVec4W = (float)curves[3].Evaluate(runningTime);
				if(!Mathf.Approximately(existingVec4W, tmpVector4.w))
					modifiedCurves.Add(new Modification(curves[3], runningTime, tmpVector4.w));
				
				break;

			case PropertyTypeInfo.Quat:
				
				tmpQuat = (Quaternion)propertyValue;
				var existingQuat4X = (float)curves[0].Evaluate(runningTime);
				if(!Mathf.Approximately(existingQuat4X, tmpQuat.x))
					modifiedCurves.Add(new Modification(curves[0], runningTime, tmpQuat.x));
				
				var existingQuat4Y = (float)curves[1].Evaluate(runningTime);
				if(!Mathf.Approximately(existingQuat4Y, tmpQuat.y))
					modifiedCurves.Add(new Modification(curves[1], runningTime, tmpQuat.y));
				
				var existingQuat4Z = (float)curves[2].Evaluate(runningTime);
				if(!Mathf.Approximately(existingQuat4Z, tmpQuat.z))
					modifiedCurves.Add(new Modification(curves[2], runningTime, tmpQuat.z));
				
				var existingQuat4W = (float)curves[3].Evaluate(runningTime);
				if(!Mathf.Approximately(existingQuat4W, tmpQuat.w))
					modifiedCurves.Add(new Modification(curves[3], runningTime, tmpQuat.w));
				
				break;

			case PropertyTypeInfo.Colour:

				tmpColour = (Color)propertyValue;
				var existingColourR = (float)curves[0].Evaluate(runningTime);
				if(!Mathf.Approximately(existingColourR, tmpColour.r))
					modifiedCurves.Add(new Modification(curves[0], runningTime, tmpColour.r));
				
				var existingColourG = (float)curves[1].Evaluate(runningTime);
				if(!Mathf.Approximately(existingColourG, tmpColour.g))
					modifiedCurves.Add(new Modification(curves[1], runningTime, tmpColour.g));
				
				var existingColourB = (float)curves[2].Evaluate(runningTime);
				if(!Mathf.Approximately(existingColourB, tmpColour.b))
					modifiedCurves.Add(new Modification(curves[2], runningTime, tmpColour.b));
				
				var existingColourA = (float)curves[3].Evaluate(runningTime);
				if(!Mathf.Approximately(existingColourA, tmpColour.a))
					modifiedCurves.Add(new Modification(curves[3], runningTime, tmpColour.a));
				
				break;

			}

			return modifiedCurves;
		}
	
		public void StoreBaseState()
		{
			object propertyValue = null;
			if(fieldInfo != null)
				propertyValue = fieldInfo.GetValue(component);
			if(propertyInfo != null)
				propertyValue = propertyInfo.GetValue(component, null);
			
			switch(propertyType)
			{
			case PropertyTypeInfo.Int:
				baseInt = (int)propertyValue;
				break;
			case PropertyTypeInfo.Long:
				baseLong = (long)propertyValue;
				break;
			case PropertyTypeInfo.Float:
				baseFloat = (float)propertyValue;
				break;
			case PropertyTypeInfo.Double:
				baseDouble = (double)propertyValue;
				break;
			case PropertyTypeInfo.Bool:
				baseBool = (bool)propertyValue;
				break;
			case PropertyTypeInfo.Vec2:
				baseVector2 = (Vector2)propertyValue;
				break;
			case PropertyTypeInfo.Vec3:
				baseVector3 = (Vector3)propertyValue;
				break;
			case PropertyTypeInfo.Vec4:
				baseVector4 = (Vector4)propertyValue;
				break;
			case PropertyTypeInfo.Quat:
				baseQuat = (Quaternion)propertyValue;
				break;
			case PropertyTypeInfo.Colour:
				baseColour = (Color)propertyValue;
				break;
			}
		}
	
		public void RestoreBaseState()
		{
			object setValue = null;
	
			switch(propertyType)
			{
			case PropertyTypeInfo.Int:
				setValue = baseInt;
				break;
			case PropertyTypeInfo.Long:
				setValue = baseLong;
				break;
			case PropertyTypeInfo.Float:
				setValue = baseFloat;
				break;
			case PropertyTypeInfo.Double:
				setValue = baseDouble;
				break;
			case PropertyTypeInfo.Bool:
				setValue = baseBool;
				break;
			case PropertyTypeInfo.Vec2:
				setValue = baseVector2;
				break;
			case PropertyTypeInfo.Vec3:
				setValue = baseVector3;
				break;
			case PropertyTypeInfo.Vec4:
				setValue = baseVector4;
				break;
			case PropertyTypeInfo.Quat:
				setValue = baseQuat;
				break;
			case PropertyTypeInfo.Colour:
				setValue = baseColour;
				break;
			}
	
			// Great, we sampled the curves, set some values.
			if(fieldInfo != null)
				fieldInfo.SetValue(component, setValue);
			if(propertyInfo != null)
				propertyInfo.SetValue(component, setValue, null);
		}
		
		/// <summary>
		/// Map an internal type to our PropertyTypeInfo
		/// </summary>
		public static PropertyTypeInfo GetMappedType(Type type)
		{
			if(type == typeof(int))
				return PropertyTypeInfo.Int;
			if(type == typeof(long))
				return PropertyTypeInfo.Long;
			if(type == typeof(float) || type == typeof(System.Single))
				return PropertyTypeInfo.Float;
			if(type == typeof(double))
				return PropertyTypeInfo.Double;
			if(type == typeof(bool))
				return PropertyTypeInfo.Bool;
			if(type == typeof(Vector2))
				return PropertyTypeInfo.Vec2;
			if(type == typeof(Vector3))
				return PropertyTypeInfo.Vec3;
			if(type == typeof(Vector4))
				return PropertyTypeInfo.Vec4;
			if(type == typeof(Quaternion))
				return PropertyTypeInfo.Quat;
			if(type == typeof(Color))
				return PropertyTypeInfo.Colour;
			
			return PropertyTypeInfo.None;
		}
		
		/// <summary>
		/// Map an PropertyTypeInfo to a Type
		/// </summary>
		public static Type GetMappedType(PropertyTypeInfo type)
		{
			if(type == PropertyTypeInfo.Int)
				return typeof(int);
			if(type == PropertyTypeInfo.Long)
				return typeof(long);
			if(type == PropertyTypeInfo.Float)
				return typeof(float);
			if(type == PropertyTypeInfo.Double)
				return typeof(double);
			if(type == PropertyTypeInfo.Bool)
				return typeof(bool);
			if(type == PropertyTypeInfo.Vec2)
				return typeof(Vector2);
			if(type == PropertyTypeInfo.Vec3)
				return typeof(Vector3);
			if(type == PropertyTypeInfo.Vec4)
				return typeof(Vector4);
			if(type == PropertyTypeInfo.Quat)
				return typeof(Quaternion);
			if(type == PropertyTypeInfo.Colour)
				return typeof(Color);
			
			return null;
		}

		private float GetValue(int curveIndex)
		{
			object propertyValue = null;
			if(fieldInfo != null)
				propertyValue = fieldInfo.GetValue(component);
			if(propertyInfo != null)
				propertyValue = propertyInfo.GetValue(component, null);

			switch(GetMappedType(propertyValue.GetType()))
			{
			case PropertyTypeInfo.Int:
				return (int)propertyValue;
			case PropertyTypeInfo.Long:
				return (long)propertyValue;
			case PropertyTypeInfo.Float:
				return (float)propertyValue;
			case PropertyTypeInfo.Double:
				return (float)((double)propertyValue);
			case PropertyTypeInfo.Bool:
				bool value =(bool)(propertyValue);
				float result = 0.0f;
				if(value == true)
					result = 1.0f;
				return result;
			case PropertyTypeInfo.Vec2:
				return ((Vector2)propertyValue)[curveIndex];
			case PropertyTypeInfo.Vec3:
				return ((Vector3)propertyValue)[curveIndex];
			case PropertyTypeInfo.Vec4:
				return ((Vector4)propertyValue)[curveIndex];
			case PropertyTypeInfo.Quat:
				return ((Quaternion)propertyValue)[curveIndex];
			case PropertyTypeInfo.Colour:
				return ((Color)propertyValue)[curveIndex];
			}
			throw new NotImplementedException("This should never happen");
		}
		
		private void BuildForCurrentValue(int curveIndex)
		{
			curves[curveIndex].Keys[0].Value = GetValue(curveIndex);
		}
	}
}
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

namespace WellFired
{
	public static class USPropertyMemberUtility 
	{
		public static string GetAdditionalMemberName(Type memberType, int index)
		{
			if(memberType == typeof(Vector4) || memberType == typeof(Vector3) || memberType == typeof(Vector2) || memberType == typeof(Quaternion))
			{
				if(index == 0)
					return ".x";
				if(index == 1)
					return ".y";
				if(index == 2)
					return ".z";
				if(index == 3)
					return ".w";
			}
			else if(memberType == typeof(Color))
			{
				if(index == 0)
					return ".r";
				if(index == 1)
					return ".g";
				if(index == 2)
					return ".b";
				if(index == 3)
					return ".a";
			}
			else if(memberType == typeof(int) || memberType == typeof(float) || memberType == typeof(long) || memberType == typeof(double) || memberType == typeof(bool))
				return "";
			
			throw new Exception(string.Format("Data Type {0} not supported", memberType));
		}
	
		public static int GetNumberOfMembers(Type memberType)
		{
			if(memberType == typeof(Vector4) || memberType == typeof(Quaternion) || memberType == typeof(Color))
				return 4;
			if(memberType == typeof(Vector3))
				return 3;
			if(memberType == typeof(Vector2))
				return 2;
			else if(memberType == typeof(int) || memberType == typeof(float) || memberType == typeof(long) || memberType == typeof(double) || memberType == typeof(bool))
				return 1;
	
			throw new Exception(string.Format("Data Type {0} not supported", memberType));
		}

		private static Dictionary<Type, Dictionary<string, string>> mappedInternalTypes = new Dictionary<Type, Dictionary<string, string>>()
		{
			{
				typeof(Transform),	
				new Dictionary<string, string>() 
				{
					{ "localeulerangles", "m_LocalRotation" },
					{ "eulerangles", "m_LocalRotation" },
				}
			},
			{ 	
				typeof(Camera),
				new Dictionary<string, string>() 
				{
					{ "isorthographic", "orthographic" }, 
					{ "useocclusionculling", "occlusionCulling" }
				}
			}
		};

		private static Dictionary<Type, List<string>> IgnoreList = new Dictionary<Type, List<string>>()
		{
			{ typeof(Camera), 			new List<string>() { "aspect", "eventmask", "layercullspherical", "clearstencilafterlightingpass", } },
			{ typeof(Light), 			new List<string>() { "shadowstrength", "shadowbias", "shadowsoftness", "shadowsoftnessfade", "alreadylightmapped" } },
			{ typeof(Terrain), 			new List<string>() { "basemapdistance", "useguilayout" } },
			{ typeof(AudioSource),  	new List<string>() { "time", "timesamples", "ignorelistenervolume", "ignorelistenerpause" } },
			{ typeof(ParticleSystem), 	new List<string>() { "playbackspeed", "enableemission", "emissionrate", "maxparticles" } },

			{ typeof(RectTransform),	new List<string>() { "anchoredposition3d", "offsetmin", "offsetmax", "right", "up", "forward", "haschanged" } },
			{ typeof(Canvas), 			new List<string>() { "scalefactor", "referencepixelsperunit",  } },
			{ typeof(CanvasRenderer), 	new List<string>() { "ismask" } },

			{ typeof(UnityEngine.UI.Image), 				new List<string>() { "ismask", "eventalphathreshold", "maskable" } },
			{ typeof(UnityEngine.EventSystems.EventSystem), new List<string>() { "pixeldragthreshold" } },
			{ typeof(UnityEngine.UI.RawImage), 				new List<string>() { "maskable" } },
			{ typeof(UnityEngine.UI.InputField), 			new List<string>() { "shouldhidemobileinput" } },
			{ typeof(UnityEngine.UI.Text), 					new List<string>() { "supportrichtext", "resizetextforbestfit", "resizetextminsize", "resizetextmaxsize", "maskable" } },
		};
		
		public static string GetUnityPropertyNameFromUSProperty(string propertyName, Component component)
		{
			var lowerPropertyName = propertyName.ToLower();
			if(IgnoreList.ContainsKey(component.GetType()) && IgnoreList[component.GetType()].Contains(lowerPropertyName))
			{
				return string.Empty;
			}
			
			var serializedObject = new SerializedObject(component);
			var serializedProperty = default(SerializedProperty);
			
			var propertyPath = string.Empty;
			if(mappedInternalTypes.ContainsKey(component.GetType()) && mappedInternalTypes[component.GetType()].TryGetValue(lowerPropertyName, out propertyPath))
				return propertyPath;
			
			var property = serializedObject.GetIterator();
			property.Next(true);
			while(property.Next(true))
			{
				var cleanPropertyName = property.name.Replace("m_", string.Empty).Replace(" ", "").ToLower();
				if(!cleanPropertyName.Contains(lowerPropertyName))
					continue;
				
				serializedProperty = property;
				break;
			}
			
			if(serializedProperty != default(SerializedProperty))
				return serializedProperty.name;
			
			Debug.Log(string.Format("Couldn't map Unity Internal : {0}", propertyName));
			return string.Empty;
		}
	}
}
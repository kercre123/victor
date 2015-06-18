using System;
using System.Linq;
using System.Reflection;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

#if NETFX_CORE
namespace WellFired.Shared
{
	public class ReflectionHelper : IReflectionHelper
	{
		public bool IsAssignableFrom(Type first, Type second)
		{
			return first.GetTypeInfo().IsAssignableFrom(second.GetTypeInfo());
		}
		
		public bool IsEnum(Type type)
		{
			return type.GetTypeInfo().IsEnum;
		}
		
		private IEnumerable GetBaseTypes(Type type)
		{
			yield return type;
			Type baseType;

			baseType = type.GetTypeInfo().BaseType;
			
			if (baseType != null)
			{
				foreach (var t in GetBaseTypes(baseType))
				{
					yield return t;
				}
			}
		}
		
		public PropertyInfo GetProperty(Type type, string name)
		{
			return type.GetTypeInfo().GetDeclaredProperty(name);
		}

		PropertyInfo GetNonPublicInstanceProperty(Type type, string name)
		{
			return type.GetTypeInfo().GetDeclaredProperty(name);
		}
		
		public MethodInfo GetMethod(Type type, string name)
		{
			return type.GetTypeInfo().GetDeclaredMethod(name);
		}

		public MethodInfo GetNonPublicStaticMethod(Type type, string name)
		{
			return type.GetTypeInfo().GetMethod(name, BindingFlags.NonPublic | BindingFlags.Static);
		}
		
		public FieldInfo GetField(Type type, string name)
		{
			return type.GetTypeInfo().GetDeclaredField(name);
		}
		
		public FieldInfo GetNonPublicInstanceField(Type type, string name)
		{
			return type.GetTypeInfo().GetField(name, BindingFlags.NonPublic | BindingFlags.Instance);
		}
		
		public bool IsValueType(Type type)
		{
			return type.GetTypeInfo().IsValueType;
		}

		public IEnumerable GetLoadedAssemblies()
		{
			throw new NotImplementedException();
		}
		
		public Type FindType(string fullName)
		{
			throw new NotImplementedException();
		}
	}
}
#endif
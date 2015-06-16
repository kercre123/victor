using UnityEngine;
using System;
using System.Collections;
using System.IO;

namespace WellFired.Shared
{
	public static class DirectoryExtensions 
	{
		private static readonly string mProjectPath = Application.dataPath.Remove(Application.dataPath.LastIndexOf("/Assets"), "/Assets".Length);
		
		public static string ProjectPath
		{
			get { return mProjectPath; }
			private set { ; }
		}

		public static string DirectoryOf(string file)
		{
			var result = string.Empty;
			var fileList = new DirectoryInfo(ProjectPath + "/Assets").GetFiles(file, SearchOption.AllDirectories); 
			if(fileList.Length == 1)
				result = fileList[0].DirectoryName.Substring(ProjectPath.Length, fileList[0].DirectoryName.Length - ProjectPath.Length).Replace('\\', '/') + '/';
			else
				throw new Exception(String.Format("Cannot find {0}, so cannot get the EditorAssetPath", file));
			
			return result;
		}
		
		public static string RelativePathOfProjectFile(string projectFile)
		{
			var directory = DirectoryExtensions.DirectoryOf(projectFile);
			return string.Format("{0}{1}", directory, projectFile);
		}
		
		public static string AbsolutePathOfProjectFile(string projectFile)
		{
			var relativePath = RelativePathOfProjectFile(projectFile);
			return string.Format("{0}{1}", DirectoryExtensions.ProjectPath, System.IO.Path.GetFullPath(relativePath));
		}
		
		public static void RecursivelyDeleteDirectory(string directoryToDelete)
		{
			var directories = Directory.GetDirectories(directoryToDelete);
			foreach(var directory in directories)
			{
				RecursivelyDeleteDirectory(directory);
			}
			
			var files = Directory.GetFiles(directoryToDelete);
			foreach(var file in files)
			{
				File.Delete(file);
			}
			
			Directory.Delete(directoryToDelete);
		}
		
		public static void CopyDirectory(string sourceDirectory, string destinationDirectory)
		{
			//Now Create all of the directories
			foreach(var dirPath in Directory.GetDirectories(sourceDirectory, "*", SearchOption.AllDirectories))
			{
				Directory.CreateDirectory(dirPath.Replace(sourceDirectory, destinationDirectory));
			}
			
			//Copy all the files & Replaces any files with the same name
			foreach(var newPath in Directory.GetFiles(sourceDirectory, "*.*", SearchOption.AllDirectories))
			{
				File.Copy(newPath, newPath.Replace(sourceDirectory, destinationDirectory), true);
			}
		}
	}
}
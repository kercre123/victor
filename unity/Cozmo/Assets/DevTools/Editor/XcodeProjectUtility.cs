using UnityEngine;
using System.Collections.Generic;
using System.IO;
using UnityEditor;

namespace Xcode {
  public static class XcodeProjectUtility {

    private static readonly IDAS sDAS = DAS.GetInstance(typeof(XcodeProjectUtility));

    [MenuItem("Cozmo/Xcode/Fixup Cozmo_IOS Project")]
    public static void FixupCozmoIos() {
      string original = File.ReadAllText("../ios/CozmoUnity_iOS.xcodeproj/project.pbxproj");

      var proj = XcodeProjectParser.Deserialize(original);


      proj.RemoveFolder("UnityBuild");

      proj.AddFolder("../ios", "../ios/UnityBuild/Classes", "UnityBuild/Classes");
      proj.AddFolder("../ios", "../ios/UnityBuild/Libraries", "UnityBuild/Libraries");
      proj.AddFolder("../ios", "../ios/UnityBuild/Unity-iPhone", "UnityBuild/Unity-iPhone");
      proj.AddFile("../ios", "../ios/UnityBuild/LaunchScreen-iPhoneLandscape.png", "UnityBuild/LaunchScreen-iPhoneLandscape.png");
      proj.AddFile("../ios", "../ios/UnityBuild/LaunchScreen-iPhonePortrait.png", "UnityBuild/LaunchScreen-iPhonePortrait.png");
      proj.AddFile("../ios", "../ios/UnityBuild/LaunchScreen-iPhone.xib", "UnityBuild/LaunchScreen-iPhone.xib");

      var parsed = XcodeProjectParser.Serialize(proj);
      File.WriteAllText("../ios/CozmoUnity_iOS.xcodeproj/project.pbxproj", parsed);
    }

    [MenuItem("Cozmo/Xcode/Test Make Relative Folder")]
    public static void TestMakeRelativeFolder() {

      string one = "Assets/HubWorld";
      string two = "../ios";

      Debug.Log(MakeRelativePath(one, two));

      one = "Assets/HubWorld/";
      two = "../ios/";

      Debug.Log(MakeRelativePath(one, two));

      one = "../ios/UnityBuild/Classes/CrashReporter.h";
      two = "../ios";

      Debug.Log(MakeRelativePath(one, two));

    }


    public enum XcodeCategory {
      Sources,
      Resources,
      Frameworks
    }

    public class FileTypeDefinition {
      public string Extension;
      public string DefaultSourceTree;
      public XcodeCategory Category;
      public string LastKnownFileType;
      public int? FileEncoding;
      public bool ExcludeFromBuildPhase;
    }

    public static List<FileTypeDefinition> FileTypeList = new List<FileTypeDefinition> {
      // Frameworks
      new FileTypeDefinition {
        Extension = "a",
        DefaultSourceTree = "SOURCE_ROOT",
        Category = XcodeCategory.Frameworks,
        LastKnownFileType = "archive.ar",
      },
      new FileTypeDefinition {
        Extension = "framework",
        DefaultSourceTree = "SDKROOT",
        Category = XcodeCategory.Frameworks,
        LastKnownFileType = "wrapper.framework"
      },
      new FileTypeDefinition {
        Extension = "dylib",
        DefaultSourceTree = "SDKROOT",
        Category = XcodeCategory.Frameworks,
        LastKnownFileType = "compiled.mach-o.dylib"
      },
      // Sources
      new FileTypeDefinition {
        Extension = "s",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Sources,
        LastKnownFileType = "sourcecode.asm",
        FileEncoding = 4
      },
      new FileTypeDefinition {
        Extension = "pch",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Sources,
        LastKnownFileType = "sourcecode.c.h",
        FileEncoding = 4,
        ExcludeFromBuildPhase = true
      },
      new FileTypeDefinition {
        Extension = "h",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Sources,
        LastKnownFileType = "sourcecode.c.h",
        FileEncoding = 4,
        ExcludeFromBuildPhase = true
      },
      new FileTypeDefinition {
        Extension = "m",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Sources,
        LastKnownFileType = "sourcecode.c.objc",
        FileEncoding = 4
      },
      new FileTypeDefinition {
        Extension = "mm",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Sources,
        LastKnownFileType = "sourcecode.cpp.objc",
        FileEncoding = 4
      },
      new FileTypeDefinition {
        Extension = "cpp",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Sources,
        LastKnownFileType = "sourcecode.cpp.cpp",
        FileEncoding = 4
      },
      // Resources
      new FileTypeDefinition {
        Extension = "png",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Resources,
        LastKnownFileType = "image.png",
      },
      new FileTypeDefinition {
        Extension = "xib",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Resources,
        LastKnownFileType = "file.xib",
        FileEncoding = 4
      },
      new FileTypeDefinition {
        Extension = "plist",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Resources,
        LastKnownFileType = "text.plist.xml",
        FileEncoding = 4
      },
      new FileTypeDefinition {
        Extension = "strings",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Resources,
        LastKnownFileType = "text.plist.strings",
        FileEncoding = 4
      },
      new FileTypeDefinition {
        Extension = "xcassets",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Resources,
        LastKnownFileType = "folder.assetcatalog"
      },
      new FileTypeDefinition {
        Extension = "xcconfig",
        DefaultSourceTree = "<group>",
        Category = XcodeCategory.Resources,
        LastKnownFileType = "text.xcconfig"
      },
          
      // TODO: Add any FileTypes that I've missed
    };

    private static string NewGuid() {
      return System.Guid.NewGuid().ToString().Replace("-", "").ToUpper().Substring(0, 24);
    }
    private static FileId NewFileId(string name, string group = null) {
      return new FileId() { Value = NewGuid(), FileName = name, Group = group };
    }

    private static PBXGroup EnsureFolderExists(this XcodeProject project, string groupId, string relativePath) {

      string[] groupPath = relativePath.Split('/');

      int index = 0;
      string lastFolderId = groupId;
      while (index < groupPath.Length) {
        string folder = groupPath[index];
        index++;
        if(string.IsNullOrEmpty(folder)) {
          continue;
        }

        var group = project.objects.PBXGroupSection.Get(lastFolderId);

        var fileId = group.children.GetFileId(folder);
        if (fileId == null) { // Create a new folder

          fileId = NewFileId(folder);
          var newGroup = new PBXGroup() { name = folder, sourceTree = "<group>" };

          project.objects.PBXGroupSection.Add(new XcodeNamedVariable<FileId, PBXGroup> {
            Name = fileId,
            Value = newGroup
          });

          group.children.Add(fileId);
        }
        lastFolderId = fileId.Value;
      }

      return project.objects.PBXGroupSection.Get(lastFolderId);
    }

    private static PBXGroup GetExistingFolder(this XcodeProject project, string groupId, string relativePath) {

      string[] groupPath = relativePath.Split('/');

      int index = 0;
      string lastFolderId = groupId;
      while (index < groupPath.Length) {
        string folder = groupPath[index];
        index++;
        if(string.IsNullOrEmpty(folder)) {
          continue;
        }

        var group = project.objects.PBXGroupSection.Get(lastFolderId);

        var fileId = group.children.GetFileId(folder);
        if (fileId == null) { // break out early
          return null;
        }
        lastFolderId = fileId.Value;
      }

      return project.objects.PBXGroupSection.Get(lastFolderId);
    }

    public static void AddFolder(this XcodeProject project, string projectRootPath, string fileSystemFolderPath, string projectFolderPath) {

      var projectRoot = project.objects.PBXProjectSection.Only().mainGroup;

      var folderGroup = project.EnsureFolderExists(projectRoot.Value, projectFolderPath);

      // this should be the folder which will get the contents of our folder

      foreach (var file in Directory.GetFiles(fileSystemFolderPath)) {
        project.AddFile(projectRootPath, folderGroup, file);
      }

      foreach (var directoryName in Directory.GetDirectories(fileSystemFolderPath)) {
        var directory = directoryName.TrimEnd('/');
        var extension = Path.GetExtension(directory);

        // Some folders are treated as files, like frameworks and image folders
        if (!string.IsNullOrEmpty(extension)) {
          project.AddFile(projectRootPath, folderGroup, directory);
        }
        else {          
          var folderName = Path.GetFileName(directory);

          if (folderName.StartsWith(".")) {
            continue;
          }
          project.AddFolder(projectRootPath, directory, Path.Combine(projectFolderPath, folderName));
        }
      }
    }

    public static void AddFile(this XcodeProject project, string projectRootPath, string fileSystemPath, string projectPath) {

      var projectRoot = project.objects.PBXProjectSection.Only().mainGroup;

      var folderGroup = project.EnsureFolderExists(projectRoot.Value, Path.GetDirectoryName(projectPath));

      // this should be the folder which will get the contents of our folder

      project.AddFile(projectRootPath, folderGroup, fileSystemPath);
    }

    private static void AddFile(this XcodeProject project, string projectRootPath, PBXGroup group, string path) {
      string name = Path.GetFileName(path);

      if (name.StartsWith(".")) {
        return;
      }

      var extension = Path.GetExtension(path).TrimStart('.');

      var fileType = FileTypeList.Find(x => x.Extension == extension);


      if (fileType == null) {
        sDAS.Error("Could Not Determine FileType for Extension " + extension);
        return;
      }

      var fileId = NewFileId(name);

      group.children.Add(fileId);

      var fileReference = new PBXFileReference() { fileEncoding = fileType.FileEncoding, lastKnownFileType = fileType.LastKnownFileType, path = MakeRelativePath(path, projectRootPath), name = name, sourceTree = "<group>" };

      project.objects.PBXFileReferenceSection.Add(new XcodeNamedVariable<FileId, PBXFileReference>() {
        Name = fileId,
        Value = fileReference
      });


      var buildFileId = NewFileId(name, fileType.Category.ToString());
      var buildFile = new PBXBuildFile() { fileRef = fileId };

      project.objects.PBXBuildFileSection.Add(new XcodeNamedVariable<FileId, PBXBuildFile>() {
        Name = buildFileId,
        Value = buildFile
      });

      if (!fileType.ExcludeFromBuildPhase) {
        switch (fileType.Category) {
        case XcodeCategory.Frameworks:
          project.objects.PBXFrameworksBuildPhaseSection.Only().files.Add(buildFileId);
          break;
        case XcodeCategory.Resources:
          project.objects.PBXResourcesBuildPhaseSection.Only().files.Add(buildFileId);
          break;
        case XcodeCategory.Sources:
          project.objects.PBXSourcesBuildPhaseSection.Only().files.Add(buildFileId);
          break;
        }
      }
    }

    private static string MakeRelativePath(string path, string workingDirectory) {

      var fullPath = Path.GetFullPath(path).Split('/');
      var fullWorkingDirectoryPath = Path.GetFullPath(workingDirectory).TrimEnd('/').Split('/');

      var len = Mathf.Min(fullPath.Length, fullWorkingDirectoryPath.Length);
      int indexOfFirstDifferent = 0;
      for (; 
           indexOfFirstDifferent < len && 
             fullPath[indexOfFirstDifferent] == fullWorkingDirectoryPath[indexOfFirstDifferent]; 
           indexOfFirstDifferent++);

      var copy = new string[fullPath.Length - indexOfFirstDifferent];
      System.Array.Copy(fullPath, indexOfFirstDifferent, copy, 0, copy.Length);
      var rightPath = string.Join("/", copy);
      var leftFolderCount = fullWorkingDirectoryPath.Length - indexOfFirstDifferent;

      for (int i = 0; i < leftFolderCount; i++) {
        rightPath = "../" + rightPath;
      }

      return rightPath;
    }

    public static void RemoveFile(this XcodeProject project, string projectRootPath, string path) {

      var relativePath = MakeRelativePath(path, projectRootPath);

      var fileReference = project.objects.PBXFileReferenceSection.Find(x => Path.Equals(x.Value.path.Value,relativePath));

      if (fileReference == null) {
        sDAS.Error("Could not find file " + path + " in Project");
        return;
      }

      var group = project.objects.PBXGroupSection.Find(x => x.Value.children.Contains(fileReference.Name));

      if (group != null) {
        group.Value.children.Remove(fileReference.Name);
      }

      var buildFile = project.objects.PBXBuildFileSection.Find(x => x.Value.fileRef.Equals(fileReference.Name));

      if (buildFile != null) {
        project.objects.PBXBuildFileSection.Remove(buildFile);
        project.objects.PBXResourcesBuildPhaseSection.Only().files.Remove(buildFile.Name);
        project.objects.PBXSourcesBuildPhaseSection.Only().files.Remove(buildFile.Name);
        project.objects.PBXFrameworksBuildPhaseSection.Only().files.Remove(buildFile.Name);
      }
    }

    private static void RemoveFileOrFolder(this XcodeProject project, PBXGroup group, FileId fileId) {

      group.children.Remove(fileId);

      var subFolder = project.objects.PBXGroupSection.Find(x => x.Name.Equals(fileId));

      if (subFolder != null) {
        
        for (var i = subFolder.Value.children.Count - 1; i >= 0; i--) {
          var child = subFolder.Value.children[i];
          project.RemoveFileOrFolder(subFolder.Value, child);
        }

        project.objects.PBXGroupSection.Remove(subFolder);
        return;
      }

      var fileReference = project.objects.PBXFileReferenceSection.Find(x => x.Name.Equals(fileId));


      if (fileReference != null) {
        project.objects.PBXFileReferenceSection.Remove(fileReference);
      }
      else {
        var variant = project.objects.PBXVariantGroupSection.Find(x => x.Name.Equals(fileId));

        if (variant != null) {
          project.objects.PBXVariantGroupSection.Remove(variant);
        }
        else {
          sDAS.Error("Could not find any reference to file id " + fileId);
        }
      }

      var buildFile = project.objects.PBXBuildFileSection.Find(x => x.Value.fileRef.Equals(fileId));

      if (buildFile != null) {
        project.objects.PBXBuildFileSection.Remove(buildFile);
        project.objects.PBXResourcesBuildPhaseSection.Only().files.Remove(buildFile.Name);
        project.objects.PBXSourcesBuildPhaseSection.Only().files.Remove(buildFile.Name);
        project.objects.PBXFrameworksBuildPhaseSection.Only().files.Remove(buildFile.Name);
      }
    }

    public static void RemoveFolder(this XcodeProject project, string projectFolderPath) {

      var projectRoot = project.objects.PBXProjectSection.Only().mainGroup;

      var folderPath = projectFolderPath.TrimEnd('/');

      var parentFolder = project.GetExistingFolder(projectRoot.Value, Path.GetDirectoryName(folderPath));

      if (parentFolder == null) {
        sDAS.Error("Could not find parent folder in project for " + projectFolderPath);
        return;
      }

      var fileId = parentFolder.children.GetFileId(Path.GetFileName(folderPath));

      RemoveFileOrFolder(project, parentFolder, fileId);
    }

    private static T Get<T, _>(this List<XcodeNamedVariable<_, T>> list, string key) where _ : XcodeString {
      var entry = list.Find(x => x.Name.Value == key);
      return entry != null ? entry.Value : default(T);
    }

    private static T Only<T, _>(this List<XcodeNamedVariable<_, T>> list) where _ : XcodeString {
      return list[0].Value;
    }
              
    private static FileId GetFileId(this List<FileId> list, string fileName) {
      return list.Find(x => x.FileName == fileName);
    }


  }

}

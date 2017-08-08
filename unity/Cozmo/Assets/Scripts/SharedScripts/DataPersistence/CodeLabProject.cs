using System;

namespace DataPersistence {
  public class CodeLabProject {
    public uint VersionNum;
    public Guid ProjectUUID;
    public bool IsVertical;
    public string ProjectXML;
    public string ProjectName;
    public DateTime DateTimeCreatedUTC;
    public DateTime DateTimeLastModifiedUTC;

    public CodeLabProject() {
      ProjectUUID = Guid.NewGuid();
      DateTimeCreatedUTC = DateTime.UtcNow; ;
      DateTimeLastModifiedUTC = DateTime.UtcNow;
      IsVertical = false; // TODO Modify how this gets set when implement vertical

      // History:
      // App version     VersionNum
      // 1.6             1 
      // 1.7             1
      // 2.0             2
      VersionNum = 2;
    }

    public CodeLabProject(string projectName, string projectXML) : this() {
      ProjectName = projectName;
      ProjectXML = projectXML;
    }
  }
}

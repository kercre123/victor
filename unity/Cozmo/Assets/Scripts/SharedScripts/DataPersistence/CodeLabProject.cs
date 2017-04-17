using System;

namespace DataPersistence {
  public class CodeLabProject {
    public Guid ProjectUUID;
    public DateTime DateTimeCreatedUTC;
    public DateTime DateTimeLastModifiedUTC;
    public string ProjectXML;
    public string ProjectName;

    public CodeLabProject() {
      ProjectUUID = Guid.NewGuid();
      DateTimeCreatedUTC = DateTime.UtcNow; ;
      DateTimeLastModifiedUTC = DateTime.UtcNow;
    }

    public CodeLabProject(string projectName, string projectXML) : this() {
      ProjectName = projectName;
      ProjectXML = projectXML;
    }
  }
}

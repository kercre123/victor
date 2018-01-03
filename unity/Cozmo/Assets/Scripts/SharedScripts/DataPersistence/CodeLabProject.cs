using System;
using Newtonsoft.Json;

namespace DataPersistence {
  public class CodeLabProject {
    public uint VersionNum;
    public Guid ProjectUUID;
    public bool IsVertical;
    public string ProjectXML;
    public string ProjectJSON;
    public string ProjectName;
    public string BaseDASProjectName; // DAS project name of the Anki project this was remixed from (null or empty if it wasn't)
    public DateTime DateTimeCreatedUTC;
    public DateTime DateTimeLastModifiedUTC;

    // History:
    // App version     VersionNum
    // 1.6             1 
    // 1.7             1
    // 2.0             2
    // 2.1             3
    public static uint kCurrentVersionNum = 3;

    public CodeLabProject() {
      ProjectUUID = Guid.NewGuid();
      DateTimeCreatedUTC = DateTime.UtcNow;
      DateTimeLastModifiedUTC = DateTime.UtcNow;

      VersionNum = kCurrentVersionNum;
    }

    public CodeLabProject(string projectName, string projectJSON, bool isVertical) : this() {
      ProjectName = projectName;
      ProjectJSON = projectJSON;
      IsVertical = isVertical;
    }

    public string GetSerializedJson() {
      string result = JsonConvert.SerializeObject(this, Formatting.Indented, new JsonSerializerSettings {
        TypeNameHandling = TypeNameHandling.Auto,
        MissingMemberHandling = MissingMemberHandling.Ignore
      });
      return result;
    }
  }
}

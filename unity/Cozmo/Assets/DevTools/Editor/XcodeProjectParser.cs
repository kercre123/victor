using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Text;
using System;
using Newtonsoft.Json;
using UnityEditor;
using System.IO;
using System.Linq;

namespace Xcode {
  public static class XcodeProjectParser {

    [MenuItem("Cozmo/Test Xcode Parse")]
    public static void TestParseProject() {
      string original = File.ReadAllText("../ios/CozmoUnity_iOS.xcodeproj/project.pbxproj");

      Debug.Log("Original:\n" + original);

      var proj = Deserialize(original);

      Debug.Log("Successfully Deserialized!");

      var parsed = Serialize(proj);

      Debug.Log("Parsed:\n" + parsed);

      File.WriteAllText("../ios/CozmoUnity_iOS.xcodeproj/project.pbxproj", parsed);
    }


    public static string Serialize(XcodeProject project) {
      StringBuilder sb = new StringBuilder();
      sb.AppendLine("// !$*UTF8*$!");

      try {
        Serialize("", project, sb, false);
      }
      catch {
        Debug.LogError("Error Serializing Project! Serialized so far:\n" + sb.ToString());
        throw;
      }
      sb.AppendLine();
      return sb.ToString();
    }

    private static void Serialize(string indent, object obj, StringBuilder sb, bool oneLine) {

      if (obj is XcodeProjectObjectsBody) {
        SerializeXcodeProjectObjectsBody(indent, (XcodeProjectObjectsBody)obj, sb);
      }
      else if (obj is IXcodeObject) {
        SerializeXcodeObject(indent, (IXcodeObject)obj, sb, oneLine);
      }
      else if (obj is IXcodeNamedVariable) {
        SerializeNamedVariable(indent, (IXcodeNamedVariable)obj, sb, oneLine);
      }
      else if (obj is IList) {
        SerializeList(indent, (IList)obj, sb, oneLine);
      }
      else if (obj is IDictionary) {
        SerializeDictionary(indent, (IDictionary)obj, sb, oneLine);
      }
      else if (obj is XcodeString) {
        sb.Append(obj.ToString());
      }
      else if (obj.GetType().IsValueType) {
        sb.Append(SerializeValueType(obj));
      }
      else if (Nullable.GetUnderlyingType(obj.GetType()) != null) {
        sb.Append(SerializeValueType(obj));
      }
      else {
        throw new System.Exception("Ecountered Unexpected value " + obj);
      }
    }

    private static string SerializeValueType(object obj) {
      if (obj is float || obj is double || obj is float? || obj is double?) {
        return ((double)obj).ToString(".0###########");
      }
      else {
        return obj.ToString();
      }
    }

    private static void SerializeList(string indent, IList list, StringBuilder sb, bool oneLine) {
      sb.Append("(");

      string separator = oneLine ? " " : "\n";

      var subIndent = indent + "\t";
      if (!oneLine) {
        sb.Append(separator);
      }
      foreach (var item in list) {
        if (!oneLine) {          
          sb.Append(subIndent);
        }
        Serialize(subIndent, item, sb, oneLine);
        sb.Append(",");
        sb.Append(separator);
      }
      if (!oneLine) {
        sb.Append(indent);
      }
      sb.Append(")");
    }

    private static void SerializeNamedVariable(string indent, IXcodeNamedVariable namedVariable, StringBuilder sb, bool oneLine) {
      sb.Append(indent);
      sb.Append(namedVariable.GetName().ToString());
      sb.Append(" = ");
      Serialize(indent, namedVariable.GetValue(), sb, oneLine);
      sb.AppendLine(";");
    }

    private static void SerializeXcodeObject(string indent, IXcodeObject obj, StringBuilder sb, bool oneLine) {
      oneLine |= (obj.GetType().GetCustomAttributes(typeof(XcodeSingleLineAttribute), true).Length > 0);

      var fields = obj.GetType().GetFields();

      Array.Sort(fields, XcodeFieldPriorityComparer.Instance);

      string subIndent = indent + "\t";

      sb.Append("{");

      string separator = oneLine ? " " : "\n";
      if (!oneLine) {
        sb.Append(separator);
      }

      for (int i = 0; i < fields.Length; i++) {        
        var name = fields[i].Name;
        var value = fields[i].GetValue(obj);

        if (value == null) {
          continue;
        }

        if (!oneLine) {
          sb.Append(subIndent);
        }

        sb.Append(name.ToString());
        sb.Append(" = ");
        Serialize(subIndent, value, sb, oneLine);
        sb.Append(";");
        sb.Append(separator);
      }
      if (!oneLine) {
        sb.Append(indent);
      }
      sb.Append("}");
    }

    private static void SerializeDictionary(string indent, IDictionary obj, StringBuilder sb, bool oneLine) {

      string subIndent = indent + "\t";

      sb.Append("{");

      string separator = oneLine ? " " : "\n";

      if (!oneLine) {
        sb.Append(separator);
      }

      foreach (DictionaryEntry kvp in obj) {
        var name = kvp.Key;
        var value = kvp.Value;

        if (value == null) {
          continue;
        }
        if (!oneLine) {
          sb.Append(subIndent);
        }

        sb.Append(name.ToString());
        sb.Append(" = ");
        Serialize(subIndent, value, sb, oneLine);
        sb.Append(";");
        sb.Append(separator);
      }

      if (!oneLine) {
        sb.Append(indent);
      }
      sb.Append("}");
    }

    private static void SerializeXcodeProjectObjectsBody(string indent, XcodeProjectObjectsBody body, StringBuilder sb) {
      var fields = typeof(XcodeProjectObjectsBody).GetFields();

      Array.Sort(fields, XcodeFieldPriorityComparer.Instance);

      sb.AppendLine("{");
      string subIndent = "\t" + indent;
      foreach (var field in fields) {
        var itemType = field.FieldType.GetGenericArguments()[0].GetGenericArguments()[1];

        string sectionName = itemType.Name;

        var list = field.GetValue(body) as IList;

        sb.AppendLine();
        if (list != null && list.Count > 0) {
          sb.AppendLine("/* Begin " + sectionName + " section */");
          foreach (var item in list) {
            Serialize(subIndent, item, sb, false);
          }
          sb.AppendLine("/* End " + sectionName + " section */");
        }
      }
      sb.Append(indent);
      sb.Append("}");
    }

    public static XcodeProject Deserialize(string projectString) {

      string[] words = projectString.Split(' ', '\t', '\n');

      //verify the first line is our magic string.
      if (words[0] != "//" || words[1] != "!$*UTF8*$!") {
        throw new Exception("Invalid project.pbxproj file!");
      }
      int index = 2;
      try {        
        XcodeProject project = (XcodeProject)Deserialize(typeof(XcodeProject), words, ref index);

        return project;
      }
      catch {
        Debug.LogError("Error found around " + WordsAround(words, index));
        throw;
      }
    }

    private static string WordsAround(string[] words, int index) {
      int start = Mathf.Max(0, index - 5);
      int end = Mathf.Min(index + 5, words.Length);

      string result = string.Empty;
      for (int i = start; i < end; i++) {

        result += " " + (i == index - 1 ? ">" : "") + words[i] + (i == index - 1 ? "<" : "");
      }

      return result.Substring(1);
    }

    private static void ChompWhitespace(string[] words, ref int index) {
      while(index < words.Length && string.IsNullOrEmpty(words[index]))
      {
        index++;
      }
    }

    private static object Deserialize(System.Type type, string[] words, ref int index) {

      ChompWhitespace(words, ref index);
      object result = null;
      if (type == typeof(XcodeProjectObjectsBody)) {
        result = DeserializeXcodeProjectObjectsBody(words, ref index);
      }
      else if (typeof(IXcodeObject).IsAssignableFrom(type)) {
        result = DeserializeXcodeObject(type, words, ref index);
      }
      else if (typeof(IXcodeNamedVariable).IsAssignableFrom(type)) {
        result = DeserializeNamedVariable(type, words, ref index);
      }
      else if (typeof(IList).IsAssignableFrom(type)) {
        result = DeserializeList(type, words, ref index);
      }
      else if (typeof(IDictionary).IsAssignableFrom(type)) {
        result = DeserializeDictionary(type, words, ref index);
      }
      else if (typeof(XcodeString).IsAssignableFrom(type)) {
        result = DeserializeXcodeString(type, words, ref index);
      }
      else if (Nullable.GetUnderlyingType(type) != null) {
        result = DeserializeNumber(Nullable.GetUnderlyingType(type), words, ref index);
      }
      else if (type.IsValueType) {
        result = DeserializeNumber(type, words, ref index);
      }
      else if (type == typeof(object)) {
        result = TryParseObject(words, ref index);
      }
      else {
        throw new Exception("Don't know how to deserialize type " + type.Name);
      }
      ChompWhitespace(words, ref index);
      return result;
    }

    private static XcodeString DeserializeXcodeString(Type type, string[] words, ref int index) {
      XcodeString str = Activator.CreateInstance(type) as XcodeString;

      string value = string.Empty;
      if (words[index].StartsWith("\"")) {
        string word;
        bool first = true;
        while(true) {
          word = words[index++];

          if(string.IsNullOrEmpty(word))
          {
            // we assume that all whitespace in our string literal is spaces.
            // this may or may not be a valid assumption?
            value += " ";
            continue;
          }

          int closeQuote = first ? 0 : -1;
          int lastCloseQuote = -1;
          while(true) {
            closeQuote = word.IndexOf("\"", closeQuote + 1);
            if(closeQuote == -1)
            {
              break;
            }
            bool escaped = false;
            int i = closeQuote - 1;
            while(i >= 0 && word[i] == '\\')
            {
              escaped = !escaped;
              i--;
            }
              
            if(!escaped)
            {
              lastCloseQuote = closeQuote;
            }
          }

          first = false;

          if (lastCloseQuote != -1) {
            value += " " + word.Substring(0, lastCloseQuote + 1);
            break;
          }
          else {
            value += " " + word;
          }
        }
        // use our json library to unescape the string
        try {
          value = JsonConvert.DeserializeObject<string>(value);
        }
        catch {
          Debug.LogError("Failed to unescape\n" + value);
          throw;
        }
      } else {
        value = words[index++].TrimEnd(';',',');
      }

      str.Value = value;

      // check if this string has a comment
      if (words[index].StartsWith("/*")) {
        string comment = string.Empty;
        index++;

        while (true) {
          var word = words[index++];

          if (word.StartsWith("*/")) {
            break;
          }
          comment += " " + word;
        }

        // we put an extra space on the start that we should strip off
        str.Comment = comment.Substring(1);
      }

      return str;
    }

    private static object DeserializeNumber(Type type, string[] words, ref int index) {
      if (type == typeof(int)) {
        return int.Parse(words[index++].TrimEnd(';',','));
      }
      if (type == typeof(float)) {
        return float.Parse(words[index++].TrimEnd(';',','));
      }
      if (type == typeof(long)) {
        return long.Parse(words[index++].TrimEnd(';',','));
      }
      if (type == typeof(double)) {
        return double.Parse(words[index++].TrimEnd(';',','));
      }
      throw new Exception("Unknown number type " + type.Name);
    }

    private static Regex _NumberRegex = new Regex("^-?[0-9\\.]+[;,]?$");

    private static object TryParseObject(string[] words, ref int index) {
      if (_NumberRegex.IsMatch(words[index])) {
        return DeserializeNumber(words[index].Contains(".") ? typeof(double) : typeof(long), words, ref index);
      }
      else if (words[index] == "(") {
        return DeserializeList(typeof(List<object>), words, ref index);
      }
      else {
        return DeserializeXcodeString(typeof(XcodeString), words, ref index);
      }
    }

    private static object DeserializeList(Type type, string[] words, ref int index) {
      var list = Activator.CreateInstance(type) as IList;

      var elementType = type.GetGenericArguments()[0];

      // first char is (
      words[index] = words[index].Substring(1);

      ChompWhitespace(words, ref index);
      while (!words[index].StartsWith(")")) {
        list.Add(Deserialize(elementType, words, ref index));

        ChompWhitespace(words, ref index);
      }

      // the last char is )
      index++;

      return list;
    }

    private static object DeserializeNamedVariable(Type type, string[] words, ref int index) {

      var keyType = type.GetGenericArguments()[0];
      var valueType = type.GetGenericArguments()[1];

      var obj = Activator.CreateInstance(type) as IXcodeNamedVariable;

      var key = (XcodeString)Deserialize(keyType, words, ref index);

      // next word will be " = "
      index++;

      var value = Deserialize(valueType, words, ref index);

      obj.SetName(key);
      obj.SetValue(value);

      return obj;
    }

    private static object DeserializeXcodeObject(Type type, string[] words, ref int index) {

      // first char is {
      words[index] = words[index].Substring(1);

      ChompWhitespace(words, ref index);

      var obj = Activator.CreateInstance(type);


      while (!words[index].StartsWith("}")) {

        var fieldName = words[index++];

        var field = type.GetField(fieldName);

        if (field == null) {
          throw new Exception(type.Name + " does not contain a field " + fieldName);
        }          

        //next is "="
        index++;

        field.SetValue(obj, Deserialize(field.FieldType, words, ref index));

        ChompWhitespace(words, ref index);
      }

      // last char is }
      index++;

      return obj;
    }

    private static object DeserializeDictionary(Type type, string[] words, ref int index) {
      // first char is {
      words[index] = words[index].Substring(1);

      ChompWhitespace(words, ref index);

      var obj = Activator.CreateInstance(type) as IDictionary;

      var keyType = type.GetGenericArguments()[0];
      var elementType = type.GetGenericArguments()[1];

      while (!words[index].StartsWith("}")) {

        var key = Deserialize(keyType, words, ref index);

        //next is "="
        index++;

        var value = Deserialize(elementType, words, ref index);

        obj.Add(key, value);

        ChompWhitespace(words, ref index);
      }

      // last char is }
      index++;

      return obj;
    }

    private static object DeserializeXcodeProjectObjectsBody(string[] words, ref int index) {      
      // first char is {
      words[index] = words[index].Substring(1);

      ChompWhitespace(words, ref index);

      var obj = new XcodeProjectObjectsBody();

      Type currentType = null;

      IList currentList = null;

      while (!words[index].StartsWith("}")) {

        // new Section
        if (words[index] == "/*") {
          if (words[index + 1] == "Begin") {
            var typeName = words[index + 2];

            //Debug.Log("Parsing " + typeName + "Section");
            var field = typeof(XcodeProjectObjectsBody).GetField(typeName + "Section");

            currentType = field.FieldType.GetGenericArguments()[0];
            currentList = field.GetValue(obj) as IList;
            if (currentList == null) {
              currentList = Activator.CreateInstance(field.FieldType) as IList;
              field.SetValue(obj, currentList);
            }
          }
          else {
            //Debug.Log("Ending Section");

            currentType = null;
            currentList = null;
          }
          index += 5;

          ChompWhitespace(words, ref index);
          continue;
        }

        currentList.Add(Deserialize(currentType, words, ref index));

        ChompWhitespace(words, ref index);
      }

      // last char is }
      index++;

      return obj;

    }
  }

  public class XcodeString : IEquatable<XcodeString>, IComparable {
    public string Value { get; set; }
    public virtual string Comment { get; set; }

    public XcodeString() {
      Value = null;
      Comment = null;
    }

    public XcodeString(string value) {
      Value = value;
      Comment = null;
    }

    public XcodeString(string value, string comment) {
      Value = value;
      Comment = comment;
    }

    private static Regex _QuoteRegex = new Regex("[\\$\\(\\)\\+\" \\\\;,\\*\\<\\>-]");

    public override string ToString() {
      string output = string.Empty;

      if (string.IsNullOrEmpty(Value)) {
        output = "\"\"";
      }
      else if (_QuoteRegex.IsMatch(Value)) {
        // use our json library to escape our string
        output = JsonConvert.SerializeObject(Value).Replace("\n","\\n").Replace("\t","\\t");
      }
      else {
        output = Value;
      }

      if (!string.IsNullOrEmpty(Comment)) {
        output += " /* " + Comment + " */";
      }
      return output;
    }

    #region IEquatable implementation

    public bool Equals(XcodeString other) {
      if (other == null)
        return false;

      return Value.Equals(other.Value);
    }

    #endregion

    #region IComparable implementation

    public int CompareTo(object obj) {
      var other = obj as XcodeString;
      if (other == null)
        return -1;
      return Value.CompareTo(other.Value);
    }

    #endregion

    public override bool Equals(object obj) {
      return Equals(obj as XcodeString);
    }

    public override int GetHashCode() {
      return Value.GetHashCode();
    }
  }

  public class FileId : XcodeString {

    public string FileName;

    public string Group;

    private static Regex _FileGroupRegex = new Regex("(.*) in (.*)");

    public override string Comment {
      get {
        if (string.IsNullOrEmpty(Group)) {
          return FileName;
        }
        else {
          return FileName + " in " + Group;
        }
      }
      set {
        if (value != null) {
          var match = _FileGroupRegex.Match(value);
          if (!match.Success) {
            FileName = value;
            Group = null;
          }
          else {
            FileName = match.Groups[1].Value;
            Group = match.Groups[2].Value;
          }
        }
        else {
          FileName = null;
          Group = null;
        }
      }
    }
  }

  public interface IXcodeObject { }

  public class XcodeProject : IXcodeObject {
    [XcodeFieldPriority(5)]
    public int archiveVersion;
    [XcodeFieldPriority(4)]
    public Dictionary<string, string> classes;
    [XcodeFieldPriority(3)]
    public int objectVersion;

    [XcodeFieldPriority(2)]
    public XcodeProjectObjectsBody objects;
    [XcodeFieldPriority(1)]
    public FileId rootObject;
  }

  public class XcodeProjectObjectsBody {

    [XcodeFieldPriority(15)]
    public List<XcodeNamedVariable<FileId, PBXBuildFile>> PBXBuildFileSection;

    [XcodeFieldPriority(14)]
    public List<XcodeNamedVariable<FileId, PBXContainerItemProxy>> PBXContainerItemProxySection;

    [XcodeFieldPriority(13)]
    public List<XcodeNamedVariable<FileId, PBXCopyFilesBuildPhase>> PBXCopyFilesBuildPhaseSection;

    [XcodeFieldPriority(12)]
    public List<XcodeNamedVariable<FileId, PBXFileReference>> PBXFileReferenceSection;

    [XcodeFieldPriority(11)]
    public List<XcodeNamedVariable<FileId, PBXFrameworksBuildPhase>> PBXFrameworksBuildPhaseSection;

    [XcodeFieldPriority(10)]
    public List<XcodeNamedVariable<FileId, PBXGroup>> PBXGroupSection;

    [XcodeFieldPriority(9)]
    public List<XcodeNamedVariable<FileId, PBXNativeTarget>> PBXNativeTargetSection;

    [XcodeFieldPriority(8)]
    public List<XcodeNamedVariable<FileId, PBXProject>> PBXProjectSection;

    [XcodeFieldPriority(7)]
    public List<XcodeNamedVariable<FileId, PBXResourcesBuildPhase>> PBXResourcesBuildPhaseSection;

    [XcodeFieldPriority(6)]
    public List<XcodeNamedVariable<FileId, PBXShellScriptBuildPhase>> PBXShellScriptBuildPhaseSection;

    [XcodeFieldPriority(5)]
    public List<XcodeNamedVariable<FileId, PBXSourcesBuildPhase>> PBXSourcesBuildPhaseSection;

    [XcodeFieldPriority(4)]
    public List<XcodeNamedVariable<FileId, PBXTargetDependency>> PBXTargetDependencySection;

    [XcodeFieldPriority(3)]
    public List<XcodeNamedVariable<FileId, PBXVariantGroup>> PBXVariantGroupSection;

    [XcodeFieldPriority(2)]
    public List<XcodeNamedVariable<FileId, XCBuildConfiguration>> XCBuildConfigurationSection;

    [XcodeFieldPriority(1)]
    public List<XcodeNamedVariable<FileId, XCConfigurationList>> XCConfigurationListSection;

  }

  public interface IXcodeNamedVariable {
    XcodeString GetName();
    object GetValue();

    void SetName(XcodeString str);
    void SetValue(object value);
  }
  public class XcodeNamedVariable<T,U> : IXcodeNamedVariable where T : XcodeString
  {
    public T Name;
    public U Value;
    public XcodeString GetName() { 
      return Name; 
    }
    public object GetValue() {
      return Value;
    }

    public void SetName(XcodeString str) {
      Name = (T)str;
    }
    public void SetValue(object value) {
      Value = (U)value;
    }
  }

  public class PBXObject : IXcodeObject {
    // we always want to force this first
    [XcodeFieldPriority(int.MaxValue)]
    public XcodeString isa;

    public PBXObject()
    {
      isa = new XcodeString(GetType().Name);
    }
  }

  [XcodeSingleLine]
  public class PBXBuildFile : PBXObject {
    [XcodeFieldPriority(2)]
    public FileId fileRef;
    [XcodeFieldPriority(1)]
    public Dictionary<XcodeString, List<XcodeString>> settings;
  }

  [XcodeSingleLine]
  public class PBXFileReference : PBXObject {
    
    [XcodeFieldPriority(7)]
    public int? fileEncoding;
    [XcodeFieldPriority(6)]
    public XcodeString lastKnownFileType;
    [XcodeFieldPriority(5)]
    public XcodeString explicitFileType;
    [XcodeFieldPriority(4)]
    public int? includeInIndex;
    [XcodeFieldPriority(3)]
    public XcodeString name;
    [XcodeFieldPriority(2)]
    public XcodeString path;
    [XcodeFieldPriority(1)]
    public XcodeString sourceTree;
  }
  
  public class PBXContainerItemProxy : PBXObject {
    [XcodeFieldPriority(4)]
    public FileId containerPortal;
    [XcodeFieldPriority(3)]
    public int proxyType;
    [XcodeFieldPriority(2)]
    public XcodeString remoteGlobalIDString;
    [XcodeFieldPriority(1)]
    public XcodeString remoteInfo;
  }

  public class PBXBuildPhase : PBXObject {
    // space these out so we can put other stuff in between
    [XcodeFieldPriority(30)]
    public long buildActionMask;
    [XcodeFieldPriority(20)]
    public List<FileId> files;
    [XcodeFieldPriority(10)]
    public int runOnlyForDeploymentPostprocessing;
  }

  public class PBXCopyFilesBuildPhase : PBXBuildPhase {
    [XcodeFieldPriority(22)]
    public XcodeString dstPath;
    [XcodeFieldPriority(21)]
    public int dstSubfolderSpec;
    [XcodeFieldPriority(11)]
    public XcodeString name;
  }

  public class PBXFrameworksBuildPhase : PBXBuildPhase { }

  public class PBXGroup : PBXObject {
    [XcodeFieldPriority(4)]
    public List<FileId> children = new List<FileId>();
    [XcodeFieldPriority(3)]
    public XcodeString name;
    [XcodeFieldPriority(2)]
    public XcodeString path;
    [XcodeFieldPriority(1)]
    public XcodeString sourceTree;
  }

  public class PBXNativeTarget : PBXObject {
    [XcodeFieldPriority(8)]
    public FileId buildConfigurationList;
    [XcodeFieldPriority(7)]
    public List<FileId> buildPhases = new List<FileId>();
    [XcodeFieldPriority(6)]
    public List<FileId> buildRules = new List<FileId>();
    [XcodeFieldPriority(5)]
    public List<FileId> dependencies = new List<FileId>();
    [XcodeFieldPriority(4)]
    public XcodeString name;
    [XcodeFieldPriority(3)]
    public XcodeString productName;
    [XcodeFieldPriority(2)]
    public FileId productReference;
    [XcodeFieldPriority(1)]
    public XcodeString productType;
  }
  public class PBXProject : PBXObject {
    [XcodeFieldPriority(10)]
    public PBXProjectAttributes attributes 
            = new PBXProjectAttributes();
    [XcodeFieldPriority(9)]
    public FileId buildConfigurationList;
    [XcodeFieldPriority(8)]
    public XcodeString compatibilityVersion;
    [XcodeFieldPriority(7)]
    public XcodeString developmentRegion;
    [XcodeFieldPriority(6)]
    public int hasScannedForEncodings;
    [XcodeFieldPriority(5)]
    public List<XcodeString> knownRegions = new List<XcodeString>();
    [XcodeFieldPriority(4)]
    public FileId mainGroup;
    [XcodeFieldPriority(3)]
    public XcodeString projectDirPath;
    [XcodeFieldPriority(2)]
    public XcodeString projectRoot;
    [XcodeFieldPriority(1)]
    public List<FileId> targets = new List<FileId>();
  }

  public class PBXProjectAttributes : IXcodeObject {
    public Dictionary<XcodeString, Dictionary<XcodeString, XcodeString>> TargetAttributes
            = new Dictionary<XcodeString, Dictionary<XcodeString, XcodeString>>();
  }

  public class PBXResourcesBuildPhase : PBXBuildPhase {}

  public class PBXShellScriptBuildPhase : PBXBuildPhase {
    [XcodeFieldPriority(13)]
    public List<FileId> inputPaths = new List<FileId>();
    [XcodeFieldPriority(12)]
    public XcodeString name;
    [XcodeFieldPriority(11)]
    public List<FileId> outputPaths = new List<FileId>();
    [XcodeFieldPriority(3)]
    public XcodeString shellPath;
    [XcodeFieldPriority(2)]
    public XcodeString shellScript;
    [XcodeFieldPriority(1)]
    public int showEnvVarsInLog;
  }

  public class PBXSourcesBuildPhase : PBXBuildPhase {}

  public class PBXTargetDependency : PBXObject {
    [XcodeFieldPriority(2)]
    public FileId target;
    [XcodeFieldPriority(1)]
    public FileId targetProxy;
  }

  public class PBXVariantGroup : PBXObject {
    [XcodeFieldPriority(3)]
    public List<FileId> children = new List<FileId>();
    [XcodeFieldPriority(2)]
    public XcodeString name;
    [XcodeFieldPriority(1)]
    public XcodeString sourceTree;
  }

  public class XCBuildConfiguration : PBXObject {
    [XcodeFieldPriority(3)]
    public FileId baseConfigurationReference;
    [XcodeFieldPriority(2)]
    public Dictionary<XcodeString, object> buildSettings;
    [XcodeFieldPriority(1)]
    public XcodeString name;
  }

  public class XCConfigurationList : PBXObject {
    [XcodeFieldPriority(3)]
    public List<FileId> buildConfigurations = new List<FileId>();
    [XcodeFieldPriority(2)]
    public int defaultConfigurationIsVisible;
    [XcodeFieldPriority(1)]
    public XcodeString defaultConfigurationName;
  }
    
  public class XcodeSingleLineAttribute : System.Attribute {
  }

  public class XcodeFieldPriorityAttribute : System.Attribute {
    public int Priority;

    public XcodeFieldPriorityAttribute(int priority) {
      Priority = priority;
    }
  }

  // Highest priority goes first
  public class XcodeFieldPriorityComparer : IComparer<System.Reflection.FieldInfo> {
    public static XcodeFieldPriorityComparer Instance = new XcodeFieldPriorityComparer();

    public int Compare(System.Reflection.FieldInfo x, System.Reflection.FieldInfo y) {
      int xOrder = x.GetCustomAttributes(typeof(XcodeFieldPriorityAttribute), true).Cast<XcodeFieldPriorityAttribute>().Select(z => z.Priority).FirstOrDefault();
      int yOrder = y.GetCustomAttributes(typeof(XcodeFieldPriorityAttribute), true).Cast<XcodeFieldPriorityAttribute>().Select(z => z.Priority).FirstOrDefault();

      return -xOrder.CompareTo(yOrder);
    }
  }
}
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

// Static data class for reading Cutscenes in from json.
public static class ConversationData {

  public static Dictionary<string, Conversation> Conversations;


  public struct Conversation {
    public readonly string SceneName;
    public readonly List<ConversationLine> Lines;

    public Conversation (string name, List<ConversationLine> convoList) {
      SceneName = name;
      Lines = new List<ConversationLine>();
      Lines.AddRange(convoList);
      // Only add this conversation if it is unique, output error otherwise.
      if (!Conversations.ContainsKey(SceneName)) {
        Conversations.Add(SceneName,this);
      }else {
        Debug.LogError(string.Format("Scene : {0} : already exists. All Scenes need a unique ID", name));
      }
    }
  }

  public static Conversation GetConversationFromData(string name) {
    Conversation toGet;
    if (Conversations.TryGetValue(name, out toGet)) {
      return toGet;
    }
    else {
      Debug.LogError(string.Format("Scene : {0} : doesn't exist. Returning NULL", name));
    }
    return toGet;
  }

  public struct ConversationLine {
    public readonly string lineID; // the ID of the line
    public readonly string text;  // Text shown in the Overlay
    public readonly string characterSprite; // The filepath of the sprite that the Overlay uses
    public readonly float duration; // Line Lifespan, if duration is <= 0, will not dismiss
    public readonly string voID; // Name of the VO event to be played when the line appears
    public readonly bool isRight; // Whether or not its oriented to the right side of the screen or left

    public ConversationLine(string ID, string txt, string sprt, float dur, string voString, bool right) {
      lineID = ID;
      text = txt;
      characterSprite = sprt;
      duration = dur;
      voID = voString;
      isRight = right;
    }
  }

  // JSON should be structured as so...
  // 
  // JSON to parse from file is a List of scenes
  // Scenes include a name and a List of lines
  // "name" : string : name of the scene
  // "lines" : array : list of Line objects
  //
  // Lines include the following fields
  // "lineID" : string : name of the Line, used for debugging mostly
  // "text" : string : actual text to be displayed for this line
  // "sprite" : string : name of the file corresponding to the character sprite
  // "duration" : float : how long line remains up naturally TODO: Potentially always use VO timing for duration
  // "voID" : string : the string key for the VO to play when this line appears
  // "isRight" : bool : whether the popup is left or right oriented
  public static void ParseJSON(JSONObject json) {
    
    if (Conversations == null) {
      Conversations = new Dictionary<string, Conversation>();
    }
    else {
      Conversations.Clear();
    }

    if (json.IsArray) {
      foreach (JSONObject sceneJson in json.list) {
        
        List<ConversationLine> newScene = new List<ConversationLine>();
        JSONObject linesJson = sceneJson.GetField("lines");

        if (linesJson.IsArray) {
          foreach (JSONObject line in linesJson.list) {

            string newID = "missingNO.";
            if (line.HasField("lineID")) {
              newID = line.GetField("lineID").str;
            }

            string newText = "";
            if (line.HasField("text")) {
              newText = line.GetField("text").str;
            }

            string newSprite = "";
            if (line.HasField("sprite")) {
              newSprite = line.GetField("sprite").str;
            }

            float newDuration = 0.0f;
            if (line.HasField("duration")) {
              newDuration = line.GetField("duration").n;
            }

            string newVO = "";
            if (line.HasField("voID")) {
              newVO = line.GetField("voID").str;
            }

            bool newRight = false;
            if (line.HasField("isRight")) {
              newRight = line.GetField("isRight").b;
            }

            ConversationLine newLine = new ConversationLine(newID, newText, newSprite, newDuration, newVO, newRight);
            newScene.Add(newLine);
          }
        }

        string newName = "noName";
        if (sceneJson.HasField("name")) {
          newName = sceneJson.GetField("name").str;
        }

        Conversation scene = new Conversation(newName, newScene);
      }
    }
  }

}

using UnityEngine;
using System.Collections;
using System.Collections.Generic;

// Static data class for reading Cutscenes in from json.
public static class CutsceneData {

  public static Dictionary<string, Cutscene> Cutscenes;


  public struct Cutscene {
    public readonly string SceneName;
    public readonly List<CutsceneLine> Lines;

    public Cutscene (string name, List<CutsceneLine> cutsceneList) {
      SceneName = name;
      Lines = new List<CutsceneLine>();
      Lines.AddRange(cutsceneList);
      // Only add this scene if it is unique, output error otherwise.
      if (!Cutscenes.ContainsKey(SceneName)) {
        Cutscenes.Add(SceneName,this);
      }else {
        Debug.LogError(string.Format("Scene : {0} : already exists. All Scenes need a unique ID", name));
      }
    }
  }

  public struct CutsceneLine {
    public readonly string lineID; // the ID of the line
    public readonly string text;  // Text shown in the Overlay
    public readonly string characterSprite; // The filepath of the sprite that the Overlay uses
    public readonly float duration; // Line Lifespan, if duration is <= 0, will need to be dismissed by other means
    public readonly string voID; // Name of the VO event to be played when the line appears
    public readonly bool isRight; // Whether or not its oriented to the right side of the screen or left

    public CutsceneLine(string ID, string txt, string sprt, float dur, string voString, bool right) {
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
    
    if (Cutscenes == null) {
      Cutscenes = new Dictionary<string, Cutscene>();
    }
    else {
      Cutscenes.Clear();
    }

    if (json.IsArray) {
      foreach (JSONObject sceneJson in json.list) {
        
        List<CutsceneLine> newScene = new List<CutsceneLine>();
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

            CutsceneLine newLine = new CutsceneLine(newID, newText, newSprite, newDuration, newVO, newRight);
            newScene.Add(newLine);
          }
        }

        string newName = "noName";
        if (sceneJson.HasField("name")) {
          newName = sceneJson.GetField("name").str;
        }

        Cutscene scene = new Cutscene(newName, newScene);
      }
    }
  }

}

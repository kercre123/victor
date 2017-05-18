using UnityEngine;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Newtonsoft.Json;
using DataPersistence;
using System;
using System.Collections.Generic;
using System.IO;
using Cozmo.UI;
using Cozmo.MinigameWidgets;


namespace CodeLab {

  public enum GrammarMode {
    None,
    Horizontal,
    Vertical
  }

  public class ProjectStats {
    private Guid _ProjectUUID = Guid.Empty;
    public int NumBlocks = 0;
    public int NumConnections = 0;
    public int NumGreenFlags = 0;
    public int NumRepeat = 0;
    public int NumForever = 0;
    public bool HasPendingChanges = false;

    private static int CountStringOccurences(string contents, string searchString) {
      // Count the number of unique times searchString occurs within contents
      // does not count overlapping strings, e.g. CountStringOccurences("bababa", "baba") == 1
      // (this is not needed for this use case, and would be slower to calculate)
      int firstChar = 0;
      int numMatches = 0;
      while (firstChar >= 0) {
        int nextMatch = contents.IndexOf(searchString, firstChar, StringComparison.Ordinal);
        if (nextMatch >= 0) {
          numMatches += 1;
          firstChar = nextMatch + 1;
        }
        else {
          firstChar = -1;
        }
      }
      return numMatches;
    }

    public void Update(CodeLabProject project) {
      var projectXML = project.ProjectXML;
      // Slightly hacky - instead of parsing the XML, just look for the entries we're interested in
      int numBlocks = CountStringOccurences(projectXML, "<block ");
      int numConnections = CountStringOccurences(projectXML, "<next>");
      int numGreenFlags = CountStringOccurences(projectXML, "<block type=\"event_whenflagclicked\"");
      int numRepeat = CountStringOccurences(projectXML, "<block type=\"control_repeat\"");
      int numForever = CountStringOccurences(projectXML, "<block type=\"control_forever\"");

      bool wasUpdated = (_ProjectUUID != project.ProjectUUID) ||
        (NumBlocks != numBlocks) ||
        (NumConnections != numConnections) ||
        (NumGreenFlags != numGreenFlags) ||
        (NumRepeat != numRepeat) ||
        (NumForever != numForever);

      _ProjectUUID = project.ProjectUUID;
      NumBlocks = numBlocks;
      NumConnections = numConnections;
      NumGreenFlags = numGreenFlags;
      NumRepeat = numRepeat;
      NumForever = numForever;

      if (wasUpdated) {
        // Keep track of changes, but only upload if/when the program is actually run
        HasPendingChanges = true;
      }
    }

    public void PostPendingChanges() {
      if (HasPendingChanges) {
        SessionState.DAS_Event("robot.code_lab.updated_user_project.blocks", NumBlocks.ToString(), DASUtil.FormatExtraData(NumConnections.ToString()));
        SessionState.DAS_Event("robot.code_lab.updated_user_project.repeat", NumRepeat.ToString(), DASUtil.FormatExtraData(NumForever.ToString()));
        SessionState.DAS_Event("robot.code_lab.updated_user_project.green_flags", NumGreenFlags.ToString());
        HasPendingChanges = false;
      }
    }
  }

  public class SessionState {

    public class ChallengesState {
      private System.DateTime _OpenDateTime;
      private System.DateTime _OpenSlideDateTime;
      private uint _CurrentSlide = 0;
      private uint _NumSlidesViewed = 0;
      private bool _IsOpen = false;

      public bool IsOpen() {
        return _IsOpen;
      }

      public void Open() {
        if (_IsOpen) {
          DAS.Error("Codelab.Challenges.Open.AlreadyOpen", "");
        }
        DAS_Event("robot.code_lab.start_challenge_ui", "");
        _IsOpen = true;
        _OpenDateTime = System.DateTime.UtcNow;
        _CurrentSlide = 0;
        _NumSlidesViewed = 0;
      }

      private void EndViewingSlide() {
        if (_CurrentSlide > 0) {
          double timeOnSlide_s = (System.DateTime.UtcNow - _OpenSlideDateTime).TotalSeconds;
          DAS_Event("robot.code_lab.end_challenge_slide_view", _CurrentSlide.ToString(), DASUtil.FormatExtraData(timeOnSlide_s.ToString()));
          if (timeOnSlide_s > 1.0) {
            _NumSlidesViewed += 1;
          }
        }
      }

      public void Close() {
        if (_IsOpen) {
          EndViewingSlide();
          double timeOpen_s = (System.DateTime.UtcNow - _OpenDateTime).TotalSeconds;
          DAS_Event("robot.code_lab.end_challenge_ui", _NumSlidesViewed.ToString(), DASUtil.FormatExtraData(timeOpen_s.ToString()));
          _IsOpen = false;
        }
        else {
          DAS.Error("Codelab.Challenges.Close.NotOpen", "");
        }
      }

      public void SetSlideNumber(uint slideNum) {
        if (_CurrentSlide != slideNum) {
          EndViewingSlide();
          _CurrentSlide = slideNum;
          _OpenSlideDateTime = System.DateTime.UtcNow;
          DAS_Event("robot.code_lab.start_challenge_slide_view", _CurrentSlide.ToString());
        }
      }
    }

    public class ProgramState {
      private System.DateTime _StartDateTime;
      private int _NumBlocksExecuted = 0;
      private bool _IsProgramRunning = false;
      private bool _WasProgramAborted = false;

      public bool IsProgramRunning() {
        return _IsProgramRunning;
      }

      public void Reset() {
        _NumBlocksExecuted = 0;
        _IsProgramRunning = false;
        _WasProgramAborted = false;
      }

      public void OnBlockStarted() {
        ++_NumBlocksExecuted;
      }

      public void StartProgram(GrammarMode grammar) {
        if (_IsProgramRunning) {
          DAS.Error("Codelab.StartProgram.AlreadyRunning", "");
        }
        _IsProgramRunning = true;

        _StartDateTime = System.DateTime.UtcNow;
        _NumBlocksExecuted = 0;
        _WasProgramAborted = false;

        switch (grammar) {
        case GrammarMode.None:
          DAS.Error("Codelab.StartProgram.NoneGrammar", "");
          break;
        case GrammarMode.Horizontal:
          DAS_Event("robot.code_lab.start_program_horizontal", "");
          break;
        case GrammarMode.Vertical:
          DAS_Event("robot.code_lab.start_program_vertical", "");

          break;
        }
      }

      public void EndProgram(GrammarMode grammar) {
        if (!_IsProgramRunning) {
          DAS.Error("Codelab.EndProgram.WasNotRunning", "");
        }
        _IsProgramRunning = false;

        double timeInProgram_s = (System.DateTime.UtcNow - _StartDateTime).TotalSeconds;

        switch (grammar) {
        case GrammarMode.None:
          DAS.Error("Codelab.EndProgram.NoneGrammar", "");
          break;
        case GrammarMode.Horizontal:
          if (_WasProgramAborted) {
            DAS_Event("robot.code_lab.aborted_program_horizontal", _NumBlocksExecuted.ToString(), DASUtil.FormatExtraData(timeInProgram_s.ToString()));
          }
          DAS_Event("robot.code_lab.end_program_horizontal", _NumBlocksExecuted.ToString(), DASUtil.FormatExtraData(timeInProgram_s.ToString()));
          break;
        case GrammarMode.Vertical:
          if (_WasProgramAborted) {
            DAS_Event("robot.code_lab.aborted_program_vertical", _NumBlocksExecuted.ToString(), DASUtil.FormatExtraData(timeInProgram_s.ToString()));
          }
          DAS_Event("robot.code_lab.end_program_vertical", _NumBlocksExecuted.ToString(), DASUtil.FormatExtraData(timeInProgram_s.ToString()));
          break;
        }
      }

      public void OnStopAll() {
        if (_IsProgramRunning) {
          _WasProgramAborted = true;
        }
      }
    }

    private ProjectStats _ProjectStats = new ProjectStats();
    private ProgramState _ProgramState = new ProgramState();
    private ChallengesState _ChallengesState = new ChallengesState();
    private System.DateTime _EnterCodeLabDateTime;
    private System.DateTime _EnterCurrentGrammarModeDateTime;
    private GrammarMode _CurrentGrammar = GrammarMode.None;
    private int _NumProgramsRun = 0;
    private int _NumProgramsRunInMode = 0;
    private bool _ReceivedGreenFlagEvent = false;

    public bool IsProgramRunning() {
      return _ProgramState.IsProgramRunning();
    }

    // Wrap DAS event so we can easily log the events for debugging
    public static void DAS_Event(string eventName, string eventValue, Dictionary<string, string> extraData = null) {
      DAS.Event(eventName, eventValue, extraData);
#if false // Enable for easier debugging the DAS events sent from CodeLab
      if (extraData != null) {
        DAS.Info(eventName, "s_val='" + eventValue + "' $data='" + extraData["$data"] + "'");
      }
      else {
        DAS.Info(eventName, "s_val='" + eventValue + "'");
      }
#endif
    }

    public void StartProgram() {
      _ProjectStats.PostPendingChanges();
      _ProgramState.StartProgram(_CurrentGrammar);
      ++_NumProgramsRun;
      ++_NumProgramsRunInMode;
    }

    public void EndProgram() {
      _ProgramState.EndProgram(_CurrentGrammar);
    }

    public void OnGreenFlagClicked() {
      DAS_Event("robot.code_lab.green_flag_clicked", "");
      _ReceivedGreenFlagEvent = true;
    }

    public void OnStopAll() {
      if (_ReceivedGreenFlagEvent) {
        // This was sent from the greenflag, and isn't a normal stop request
        _ReceivedGreenFlagEvent = false;
      }
      else {
        DAS_Event("robot.code_lab.stop_all", "");
      }

      _ProgramState.OnStopAll();
    }

    private void Reset() {
      _ProgramState.Reset();
      if (_CurrentGrammar != GrammarMode.None) {
        DAS.Error("Codelab.ResetActiveGrammar", "Should exit mode before reset!");
      }
      _NumProgramsRun = 0;
      _NumProgramsRunInMode = 0;
      _ReceivedGreenFlagEvent = false;
    }

    public void StartSession() {
      DAS_Event("robot.code_lab.open", "");
      Reset();
      _EnterCodeLabDateTime = System.DateTime.UtcNow;
      SetGrammarMode(GrammarMode.Horizontal);
    }

    public void EndSession() {
      SetGrammarMode(GrammarMode.None);
      double timeInCodeLab_s = (System.DateTime.UtcNow - _EnterCodeLabDateTime).TotalSeconds;
      DAS_Event("robot.code_lab.close", _NumProgramsRun.ToString(), DASUtil.FormatExtraData(timeInCodeLab_s.ToString()));
    }

    private void ExitGrammarMode() {
      double timeInCodeLabMode_s = (System.DateTime.UtcNow - _EnterCurrentGrammarModeDateTime).TotalSeconds;

      switch (_CurrentGrammar) {
      case GrammarMode.None:
        DAS.Error("Codelab.ExitNoneGrammar", "");
        break;
      case GrammarMode.Horizontal:
        DAS_Event("robot.code_lab.end_horizontal", _NumProgramsRunInMode.ToString(), DASUtil.FormatExtraData(timeInCodeLabMode_s.ToString()));
        break;
      case GrammarMode.Vertical:
        DAS_Event("robot.code_lab.end_vertical", _NumProgramsRunInMode.ToString(), DASUtil.FormatExtraData(timeInCodeLabMode_s.ToString()));
        break;
      }

      _CurrentGrammar = GrammarMode.None;
    }

    private void EnterGrammarMode(GrammarMode newGrammar) {
      if (_CurrentGrammar != GrammarMode.None) {
        DAS.Error("Codelab.EnterAlreadyRunningGrammar", "CurrentGrammar = " + _CurrentGrammar);
      }

      switch (newGrammar) {
      case GrammarMode.None:
        DAS.Error("Codelab.EnterNoneGrammar", "");
        break;
      case GrammarMode.Horizontal:
        DAS_Event("robot.code_lab.start_horizontal", "");
        break;
      case GrammarMode.Vertical:
        DAS_Event("robot.code_lab.start_vertical", "");
        break;
      }

      _CurrentGrammar = newGrammar;
      _NumProgramsRunInMode = 0;
      _EnterCurrentGrammarModeDateTime = System.DateTime.UtcNow;
    }

    public void SetGrammarMode(GrammarMode newGrammar) {
      if (_CurrentGrammar != newGrammar) {
        if (_CurrentGrammar != GrammarMode.None) {
          ExitGrammarMode();
        }
        if (newGrammar != GrammarMode.None) {
          EnterGrammarMode(newGrammar);
        }
      }
    }

    public void ScratchBlockEvent(string blockEventName, Dictionary<string, string> extraData = null) {
      DAS_Event("robot.code_lab.scratch_block", blockEventName, extraData);
      _ProgramState.OnBlockStarted();
    }

    public void OnUpdatedProject(CodeLabProject project) {
      _ProjectStats.Update(project);
    }

    public void OnCreatedProject(CodeLabProject project) {
      PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;
      DAS_Event("robot.code_lab.created_project", defaultProfile.CodeLabProjects.Count.ToString());
    }

    public void OnChallengesOpen() {
      _ChallengesState.Open();
    }

    public void OnChallengesClose() {
      _ChallengesState.Close();
    }

    public void OnChallengesSetSlideNumber(uint slideNum) {
      _ChallengesState.SetSlideNumber(slideNum);
    }
  }
}
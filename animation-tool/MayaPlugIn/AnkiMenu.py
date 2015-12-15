import sys
import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx
import maya.cmds as cmds
import maya.mel as mel
import json
import math
import os
from operator import itemgetter

#To setup add "MAYA_PLUG_IN_PATH = <INSERT PATH TO COZMO GAME HERE>/cozmo-game/animation-tool/MayaPlugIn" in "~/Library/Preferences/Autodesk/maya/2016/Maya.env"
#In maya "Windows -> Setting/Preferences -> Plug-In Manager" and check under AnkiMenu.py" hit load and auto-load and an Anki menu will appear on the main menu

kPluginCmdName = "AnkiAnimExport"
kShortFlagName = "oj"
kLongFlagName = "open_json"
#something like ~/cozmo-game/lib/anki/products-cozmo-assets/animations
g_AnkiExportPath = ""
g_PrevMenuName = ""
g_ProceduralFaceKeyFrames = []
ANIM_FPS = 30
MAX_FRAMES = 10000
NUM_PROCEDURAL_FRAMES = 19
DATA_NODE_NAME = "x:data_node"

# mapping from  cozmo-game\unity\cozmo\assets\scripts\generated\clad\types\proceduraleyeparameters.cs
#which could have probably been included, but we'd still need a way to map those to the maya names.

#WARNING!!!!! In vision engineering "left" means screen left, but in animation it's the characters left
# so in the script ( and bakeResults transforms ) we flip left and right
# END HACK WARNING
g_ProcFaceDict = { 
"FaceCenterX":{"cladName":"faceCenterX", "cladIndex":-1},
"FaceCenterY":{"cladName":"faceCenterY", "cladIndex":-1},
"FaceScaleX":{"cladName":"faceScaleX", "cladIndex":-1}, 
"FaceScaleY":{"cladName":"faceScaleY", "cladIndex":-1}, 
"FaceAngle":{"cladName":"faceAngle", "cladIndex":-1}, 

"LeftEyeCenterX":{"cladName":"rightEye", "cladIndex":0}, 
"LeftEyeCenterY":{"cladName":"rightEye", "cladIndex":1}, 
"LeftEyeScaleX":{"cladName":"rightEye", "cladIndex":2}, 
"LeftEyeScaleY":{"cladName":"rightEye", "cladIndex":3}, 
"LeftEyeAngle":{"cladName":"rightEye", "cladIndex":4}, 
"Eye_Corner_L_Inner_Lower_X":{"cladName":"rightEye", "cladIndex":5}, 
"Eye_Corner_L_Inner_Lower_Y":{"cladName":"rightEye", "cladIndex":6}, 
"Eye_Corner_L_Inner_Upper_X":{"cladName":"rightEye", "cladIndex":7},
"Eye_Corner_L_Inner_Upper_Y":{"cladName":"rightEye", "cladIndex":8},
"Eye_Corner_L_Outer_Upper_X":{"cladName":"rightEye", "cladIndex":9}, 
"Eye_Corner_L_Outer_Upper_Y":{"cladName":"rightEye", "cladIndex":10}, 
"Eye_Corner_L_Outer_Lower_X":{"cladName":"rightEye", "cladIndex":11}, 
"Eye_Corner_L_Outer_Lower_Y":{"cladName":"rightEye", "cladIndex":12},
"LeftEyeUpperLidY":{"cladName":"rightEye", "cladIndex":13}, 
"LeftEyeUpperLidAngle":{"cladName":"rightEye", "cladIndex":14}, 
"Left_Eye_Upper_Lid_Bend":{"cladName":"rightEye", "cladIndex":15}, 
"LeftEyeLowerLidY":{"cladName":"rightEye", "cladIndex":16}, 
"LeftEyeLowerLidAngle":{"cladName":"rightEye", "cladIndex":17}, 
"Left_Eye_Lower_Lid_Bend":{"cladName":"rightEye", "cladIndex":18}, 

"RightEyeCenterX":{"cladName":"leftEye", "cladIndex":0}, 
"RightEyeCenterY":{"cladName":"leftEye", "cladIndex":1}, 
"RightEyeScaleX":{"cladName":"leftEye", "cladIndex":2}, 
"RightEyeScaleY":{"cladName":"leftEye", "cladIndex":3}, 
"RightEyeAngle":{"cladName":"leftEye", "cladIndex":4}, 
"Eye_Corner_R_Inner_Lower_X":{"cladName":"leftEye", "cladIndex":5}, 
"Eye_Corner_R_Inner_Lower_Y":{"cladName":"leftEye", "cladIndex":6}, 
"Eye_Corner_R_Inner_Upper_X":{"cladName":"leftEye", "cladIndex":7},
"Eye_Corner_R_Inner_Upper_Y":{"cladName":"leftEye", "cladIndex":8},
"eyeCorner_R_outerUpper_X":{"cladName":"leftEye", "cladIndex":9}, 
"Eye_Corner_R_Outer_Upper_Y":{"cladName":"leftEye", "cladIndex":10},
"Eye_Corner_R_Outer_Lower_X":{"cladName":"leftEye", "cladIndex":11}, 
"Eye_Corner_R_Outer_Lower_Y":{"cladName":"leftEye", "cladIndex":12},
"RightEyeUpperLidY":{"cladName":"leftEye", "cladIndex":13}, 
"RightEyeUpperLidAngle":{"cladName":"leftEye", "cladIndex":14}, 
"Right_Eye_Upper_Lid_Bend":{"cladName":"leftEye", "cladIndex":15}, 
"RightEyeLowerLidY":{"cladName":"leftEye", "cladIndex":16}, 
"RightEyeLowerLidAngle":{"cladName":"leftEye", "cladIndex":17}, 
"Right_Eye_Lower_Lid_Bend":{"cladName":"leftEye", "cladIndex":18}, 
}

def SetAnkiExportPath(item):
    #default to last temp file location
    global g_AnkiExportPath
    if( g_AnkiExportPath == ""):
        g_AnkiExportPath = cmds.file( query=True, lastTempFile=True)
    filenames = cmds.fileDialog2(fileMode=3, caption="Export Directory")
    if( len(filenames) > 0):
        g_AnkiExportPath = filenames[0]
        print "Anki Set: exported path is " + g_AnkiExportPath + " .";
    else:
        print "ERROR NO directory selected"

# function that determines if strings match of a procedural string
def IsProceduralAttribute(currattr):
    global g_ProcFaceDict
    if( currattr in g_ProcFaceDict.keys()):
        return True
    return False

def GetAudioJSON():
    audioNode = cmds.timeControl(mel.eval( '$tmpVar=$gPlayBackSlider' ),query=True, sound=True)
    if( (audioNode  is not None) and (audioNode != "") ):
        audioStart = cmds.sound(audioNode,query=True, offset=True)
        audioEnd = cmds.sound(audioNode,query=True, sourceEnd=True)
        audioFile = cmds.sound(audioNode,query=True, file=True)

        audioPathSplit = audioFile.split("/sounds/", 1)
        if( len(audioPathSplit) < 2 ):
            print "ERROR: no sound added, it must be in the sounds directory"
            return None
        audioFile = audioPathSplit[1]
        audio_json = {         
                        "audioName": [audioFile],
                        "volume": 1.0,
                        "triggerTime_ms": round(audioStart * 1000  / ANIM_FPS, 0),
                        "durationTime_ms": round((audioEnd - audioStart) * 1000  / ANIM_FPS, 0),
                        "Name": "RobotAudioKeyFrame"
                    }
        return audio_json
    else:
        print "No audio found"
    return None

#keeping this backwards compatible leads to some weird concatination.
def GetMovementJSON():
    global ANIM_FPS
    global DATA_NODE_NAME
    #Get a list of all the user defined attributes #dataNodeObject = "x:mech_all_ctrl"
    dataNodeObject = DATA_NODE_NAME
    if( len( cmds.ls(dataNodeObject) ) == 0 ):
        return None
    attributes = cmds.listAttr(dataNodeObject, ud=True)
    if( len( attributes ) == 0 ):
        return None
    json_arr = []
    
    keyframe_attr_data = {}
    #Movement requires all attributes be set with the known names, and all together.
    keyframe_attr_data["RadiusValues"] = cmds.keyframe( dataNodeObject, attribute = "Radius", query=True, valueChange=True)
    keyframe_attr_data["TurnValues"] = cmds.keyframe( dataNodeObject, attribute = "Turn", query=True, valueChange=True)
    keyframe_attr_data["FwdValues"] = cmds.keyframe( dataNodeObject, attribute = "Forward", query=True, valueChange=True)
    keyframe_attr_data["TimesRadius"] = cmds.keyframe( dataNodeObject, attribute = "Radius", query=True, timeChange=True)
    keyframe_attr_data["TimesTurn"] = cmds.keyframe( dataNodeObject, attribute = "Turn", query=True, timeChange=True)
    keyframe_attr_data["TimesForward"] = cmds.keyframe( dataNodeObject, attribute = "Forward", query=True, timeChange=True)
    
    if( keyframe_attr_data["TimesForward"] is None or 
        keyframe_attr_data["TimesRadius"] is None or 
        keyframe_attr_data["TimesTurn"] is None):
            print "Could not find Movement values."
            return None
    #Try to find an intersection all attributes agree on, bakeResults sometimes inserts where there shouldn't be.
    min_keyframes = min([len(keyframe_attr_data["TimesForward"]),len(keyframe_attr_data["TimesRadius"]),len(keyframe_attr_data["TimesTurn"])])

    move_data_combined = [] #object so I don't have a bunch of different arrays
    i = 1 #can skip the init frame and round off.
    valid_count = 0
    single_frame_time = 1 / ANIM_FPS
    for i in range(min_keyframes):
        valid_keyframe = {
                "Forward": round(keyframe_attr_data["FwdValues"][i]),
                "Turn": round(keyframe_attr_data["TurnValues"][i]),
                "Radius": round(keyframe_attr_data["RadiusValues"][i]),
                "Time": keyframe_attr_data["TimesForward"][i]
            }
        #if our scale has shorted a frame too much, ignore it.
        if( valid_count > 0):
            if( keyframe_attr_data["TimesForward"][i] - move_data_combined[valid_count-1]["Time"] < single_frame_time):
                continue
        move_data_combined.append( valid_keyframe );
        valid_count = valid_count + 1

    #TODO: error message if all are not keyed.
    if( len(move_data_combined) == 0):
        return None
    #Loop through all keyframes
    keyframe_count = len(move_data_combined)
    #The first keyframe always just inits things at 0
    i = 1
    for i in range(keyframe_count-1):
        #skip the ending frames and go to the start of the next bookend, we process in pairs.
        if( ( i % 2) == 0 ):
            continue
        duration = (move_data_combined[i+1]["Time"] - move_data_combined[i]["Time"]) * 1000 / ANIM_FPS
        triggerTime_ms = round((move_data_combined[i]["Time"]) * 1000  / ANIM_FPS, 0);
        durationTime_ms = round(duration, 0)
        time_in_seconds = durationTime_ms / 1000.0
        curr = {
                    "triggerTime_ms": triggerTime_ms,
                    "durationTime_ms": durationTime_ms,
                    "Name": "BodyMotionKeyFrame"
                }
        fwd_delta = move_data_combined[i+1]["Forward"] - move_data_combined[i]["Forward"]
        turn_delta = move_data_combined[i+1]["Turn"] - move_data_combined[i]["Turn"]
        radius_delta = move_data_combined[i+1]["Radius"] - move_data_combined[i]["Radius"]
        if( (fwd_delta == 0) and  
            (move_data_combined[i+1]["Radius"] == 0) and
            (turn_delta  == 0)):
            #just an empty reset frame.
            continue
        if( (fwd_delta  != 0) and  
            (move_data_combined[i+1]["Radius"]  == 0) and
            (turn_delta  == 0)):
            curr["radius_mm"] = "STRAIGHT"
            curr["speed"] = fwd_delta
        elif( (fwd_delta == 0) and  
            (move_data_combined[i+1]["Radius"] == 0) and
            (turn_delta  != 0)):
            curr["radius_mm"] = "TURN_IN_PLACE"
            # whereas in maya the turn values are in degrees. The one robot Mooly tested on turned about 360 degrees in 1.25 seconds
            # but in theory the max is 8 radians per second 
            rot_degrees_wanted = turn_delta
            curr["speed"] = round(rot_degrees_wanted / time_in_seconds)
        # Radius and turn is non-zero and forward is 0, then it's an arc
        else:
            curr["radius_mm"] = keyframe_attr_data["RadiusValues"][i+1]
            rot_radians = math.radians(turn_delta)
            arc_length = curr["radius_mm"] * rot_radians
            curr["speed"] = round(arc_length / time_in_seconds)
        json_arr.append(curr)
    return json_arr

# function that adds ( creates new or modifies existing keyframe)
def AddProceduralKeyframe(currattr,triggerTime_ms,durationTime_ms,val,frameNumber):
    # search and see if we have something at that time
    # if exists just modify currattr value else insert a blank one.
    global g_ProceduralFaceKeyFrames
    global g_ProcFaceDict
    global DATA_NODE_NAME
    frame = None
    for existing_face_frame in g_ProceduralFaceKeyFrames:
        if( existing_face_frame["triggerTime_ms"] == triggerTime_ms):
            frame = existing_face_frame
            break

    if( frame is None ):
        #add a completely empty one with logical defaults
        frame = {        
            "faceAngle": 0.0,
            "faceCenterX": 0,
            "faceCenterY": 0,
            "faceScaleX": 1.0,
            "faceScaleY": 1.0,
            "leftEye":  ([0]*NUM_PROCEDURAL_FRAMES),
            "rightEye":  ([0]*NUM_PROCEDURAL_FRAMES),
            "triggerTime_ms": triggerTime_ms,
            "durationTime_ms": durationTime_ms,
            "Name": "ProceduralFaceKeyFrame"
        }
        #Add the interpolated values for what maya thinks it is at, that way not every attribute needs to be keyed in maya
        for k, v in g_ProcFaceDict.iteritems():
            interp_val = cmds.getAttr(DATA_NODE_NAME + '.' + k,time=frameNumber)
            if( v["cladIndex"] >= 0):
                frame[v["cladName"]][v["cladIndex"]] = interp_val
            else:
                frame[v["cladName"]] = interp_val
        g_ProceduralFaceKeyFrames.append(frame)

    # mod the attribute you need
    #"Eye_Corner_L_Outer_Lower_X":{"cladName":"leftEye", "cladIndex":11}, 
    write_info = g_ProcFaceDict[currattr]
    if( write_info["cladIndex"] >= 0):
        frame[write_info["cladName"]][write_info["cladIndex"]] = val
    else:
        frame[write_info["cladName"]] = val
    return

def VerifyAnkiExportPath():
    global g_AnkiExportPath;
    if( g_AnkiExportPath == ""):
        environment_path = mel.eval("getenv ANKI_ANIM_EXPORT_PATH")
        g_AnkiExportPath = environment_path
        if( g_AnkiExportPath == ""):
            SetAnkiExportPath("Default")

def OpenJson(item):
    global g_AnkiExportPath
    VerifyAnkiExportPath();
    output_name = cmds.file(query=True, sceneName=True, shortName=True).split('.')[0]
    json_filename =g_AnkiExportPath + "/"+output_name +".json"
    print "Attempting to open: " + json_filename
    os.system("open "+json_filename)
    
#JSON printing
def ExportAnkiAnim(item):
    global g_AnkiExportPath;
    print "AnkiAnimExportStarted..."
    #Check if we've set something before this session, use a maya.env var or force dialog
    VerifyAnkiExportPath()

    global ANIM_FPS
    global MAX_FRAMES
    global DATA_NODE_NAME
    if( len( cmds.ls(DATA_NODE_NAME) ) == 0 ):
        print "ERROR: AnkiAnimExport: Must have " + DATA_NODE_NAME
        return
    dataNodeObject = cmds.ls(DATA_NODE_NAME)[0]

    #Bake the animation so that we get the maya values mapped to the cozmo face values that the datanode driven keyframes transform
    cmds.bakeResults( dataNodeObject, time=(1,cmds.playbackOptions( query=True,max=True )),simulation=True, smart=True,disableImplicitControl=True,preserveOutsideKeys=True,sparseAnimCurveBake=False,removeBakedAttributeFromLayer=False,removeBakedAnimFromLayer=False,bakeOnOverrideLayer=True,minimizeRotation=True,controlPoints=False,shape=True )   
    
    #If there were multiple anim layers, we need to make sure the latest created was selected.
    #TODO: make sure BakeResults named doesnt exist as we also clean it up by name.
    cmds.select( clear=True )
    cmds.animLayer("BakeResults",edit=True,selected=True,solo=True)
    # Cut out extra frames generated by the bake
    # Maya generates keyframes in negative time, and they are garbage.
    cmds.cutKey( dataNodeObject,time=(MAX_FRAMES,sys.maxint) )
    cmds.cutKey( dataNodeObject,time=((-sys.maxint - 1),-1) )

    #Engine Hack because the engine is still running on the robots clock, it needs to be at 33ms instead of 33.33
    #outputting at the correct maya 30fps will eventually cause drift. scaleKey -fp 0 -ts .99
    cmds.scaleKey(dataNodeObject,floatPivot=0,timeScale=0.99)
    #End engine hack to remove after Demo

    node_list = cmds.listConnections('BakeResults')
    for baked_key in node_list:
        cmds.keyTangent(baked_key, inTangentType='linear', outTangentType='linear')

    json_arr = []
    # Because in the file we store the procedural anim as one keyframe with all params
    # But in maya they are stored as multiple params, we have this system for inserting them all.
    # Global to make the function work easier and laziness
    global g_ProceduralFaceKeyFrames
    g_ProceduralFaceKeyFrames = []

    #Get a list of all the user defined attributes
    attributes = cmds.listAttr(dataNodeObject, ud=True)
    #Loop through all attributes
    for currattr in attributes:
        #Get values and times for all keyframes of the current attribute
        isProceduralFaceAttr = IsProceduralAttribute(currattr)
        Vs = cmds.keyframe( dataNodeObject, attribute = currattr, query=True, valueChange=True)
        Ts = cmds.keyframe( dataNodeObject, attribute = currattr, query=True, timeChange=True)

        #maya API thinks it's cool to return null instead of an empty list.
        if Vs is None:
            continue
        if Ts is None:
            continue
       
        i = 0;
        #Loop through all keyframes, the value is actually the next keyframe
        keyframe_count = len(Vs)
        for i in range(keyframe_count):
            triggerTime_ms = int(round((Ts[i]) * 1000  / ANIM_FPS, 0))
            #For the procedural face, the engine does a lookahead, but for the others this script does lookahead.
            # ... because they were written different times by different people
            #Try to make this consistent after Demo
            if isProceduralFaceAttr:
                AddProceduralKeyframe(currattr,triggerTime_ms,0,Vs[i],Ts[i])
            elif (currattr == "HeadAngle" or currattr == "ArmLift") and (i < keyframe_count-1): 
                #Ignore duplicate keyframes, Since these are absolutes this is Not a no-op.
                if( Vs[i] == Vs[i+1]):
                    continue
                duration = (Ts[i+1] - Ts[i]) * 1000 / ANIM_FPS 
                keyframe_value = Vs[i+1]
                curr = None
                durationTime_ms = int(round(duration, 0))
                if currattr == "HeadAngle":
                    curr = {
                        "angle_deg": keyframe_value,
                        "angleVariability_deg": 0,
                        "triggerTime_ms": triggerTime_ms,
                        "durationTime_ms": durationTime_ms,
                        "Name": "HeadAngleKeyFrame",
                    }
                elif currattr == "ArmLift":
                    curr = {          
                        "height_mm": keyframe_value,
                        "heightVariability_mm": 0,
                        "triggerTime_ms": triggerTime_ms,
                        "durationTime_ms": durationTime_ms,
                        "Name": "LiftHeightKeyFrame"
                    }

                if( curr is not None):
                    json_arr.append(curr)
    #Concat the procedural face frames which were added per attribute
    # since not every attribute needs to be keyed, it might be out of order and need sorting.
    g_ProceduralFaceKeyFrames = sorted(g_ProceduralFaceKeyFrames, key=itemgetter('triggerTime_ms')) 
    json_arr.extend(g_ProceduralFaceKeyFrames)
    #Grab the robot sounds from the main timeline, not a datanode attribute
    audio_json = GetAudioJSON()
    if( audio_json is not None):
        json_arr.append(audio_json)
    #Grab the robot movement relative to the curve
    movement_json_arr = GetMovementJSON()
    if( movement_json_arr is not None):
        json_arr.extend(movement_json_arr)
    #Deletes the results layer the script created.
    cmds.delete("BakeResults")
    cmds.delete("BakeResultsContainer")

    #Scene name with stripped off .ma extension
    output_name = cmds.file(query=True, sceneName=True, shortName=True).split('.')[0]
    # the original tool just had the whole thing in a wrapper
    json_dict = {output_name:json_arr}
    output_json = json.dumps(json_dict, sort_keys=False, indent=2, separators=(',', ': '))
    # redirects console output to a file then close
    cmds.cmdFileOutput( o=g_AnkiExportPath + "/"+output_name +".json")
    print output_json
    cmds.cmdFileOutput( c=1 )
    return

# Command
class scriptedCommand(OpenMayaMPx.MPxCommand):
    def __init__(self):
        OpenMayaMPx.MPxCommand.__init__(self)
        
    # Invoked when the command is run. ( Command so can be integrated with other scripts. )
    def doIt(self,argList):
        print "Anki Plugin called!" 
        argData = OpenMaya.MArgParser( self.syntax(), argList )
        #Run with "AnkiAnimExport -open_json"
        if argData.isFlagSet( kShortFlagName ):
            OpenJson(True)
        else:
            ExportAnkiAnim(True)

# Creator
def cmdCreator():
    return OpenMayaMPx.asMPxPtr( scriptedCommand() )

def syntaxCreator():
    ''' Defines the argument and flag syntax for this command. '''
    syntax = OpenMaya.MSyntax()
    syntax.addFlag( kShortFlagName, kLongFlagName, OpenMaya.MSyntax.kNoArg )
    return syntax

# Initialize the script plug-in
def initializePlugin(mobject):
    global g_PrevMenuName
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    print "Initting Anki plugin"
    try:
        mplugin.registerCommand( kPluginCmdName, cmdCreator, syntaxCreator )
        #create the menu on the main menu bar
        g_PrevMenuName = cmds.menu( label='Anki', parent='MayaWindow')
        cmds.menuItem( label='Export Anim', command=ExportAnkiAnim, ctrlModifier=True, keyEquivalent="e"  ) 
        cmds.menuItem( label='Set Export Path', command=SetAnkiExportPath, ctrlModifier=True, keyEquivalent="p"  ) 
    except:
        sys.stderr.write( "Failed to register command: %s\n" % kPluginCmdName )
        raise

# Uninitialize the script plug-in
def uninitializePlugin(mobject):
    global g_PrevMenuName
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    print "Uninitialize Anki plugin"
    #clean up our previous menu
    try:
        mplugin.deregisterCommand( kPluginCmdName )
        cmds.deleteUI( g_PrevMenuName, menu=True )
    except:
        sys.stderr.write( "Failed to unregister command: %s\n" % kPluginCmdName )
        
        



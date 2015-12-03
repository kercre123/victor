import sys
import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx
import maya.cmds as cmds
import maya.mel as mel
import json

#To use place in "/Users/Shared/Autodesk/maya/2016/plug-ins and In maya "Windows -> Setting/Preferences -> Plug-In Manager" and check under AnkiMenu.py" hit load

kPluginCmdName = "AnkiAnimExport"
#something like /Users/mollyjameson/cozmo-game/lib/anki/products-cozmo-assets/animations
g_AnkiExportPath = ""
g_PrevMenuName = ""
g_ProceduralFaceKeyFrames = []
ANIM_FPS = 30
g_ProcFaceDict = { 
"FaceCenterX":{"cladName":"faceCenterX", "cladIndex":-1},
"FaceCenterY":{"cladName":"faceCenterY", "cladIndex":-1},
"FaceScaleX":{"cladName":"faceScaleX", "cladIndex":-1}, 
"FaceScaleY":{"cladName":"faceScaleY", "cladIndex":-1}, 
"FaceAngle":{"cladName":"faceAngle", "cladIndex":-1}, 
"LeftEyeCenterX":{"cladName":"leftEye", "cladIndex":0}, 
"LeftEyeCenterY":{"cladName":"leftEye", "cladIndex":1}, 
"LeftEyeScaleX":{"cladName":"leftEye", "cladIndex":2}, 
"LeftEyeScaleY":{"cladName":"leftEye", "cladIndex":3}, 
"LeftEyeAngle":{"cladName":"leftEye", "cladIndex":4}, 
"RightEyeCenterX":{"cladName":"rightEye", "cladIndex":0}, 
"RightEyeCenterY":{"cladName":"rightEye", "cladIndex":1}, 
"RightEyeScaleX":{"cladName":"rightEye", "cladIndex":2}, 
"RightEyeScaleY":{"cladName":"rightEye", "cladIndex":3}, 
"RightEyeAngle":{"cladName":"rightEye", "cladIndex":4}, 
"LeftEyeUpperLidY":{"cladName":"leftEye", "cladIndex":13}, 
"LeftEyeUpperLidAngle":{"cladName":"leftEye", "cladIndex":14}, 
"Left_Eye_Upper_Lid_Bend":{"cladName":"leftEye", "cladIndex":15}, 
"LeftEyeLowerLidAngle":{"cladName":"leftEye", "cladIndex":17}, 
"Left_Eye_Lower_Lid_Bend":{"cladName":"leftEye", "cladIndex":18}, 
"RightEyeUpperLidY":{"cladName":"rightEye", "cladIndex":13}, 
"RightEyeUpperLidAngle":{"cladName":"rightEye", "cladIndex":14}, 
"Right_Eye_Upper_Lid_Bend":{"cladName":"rightEye", "cladIndex":15}, 
"RightEyeLowerLidY":{"cladName":"rightEye", "cladIndex":16}, 
"RightEyeLowerLidAngle":{"cladName":"rightEye", "cladIndex":17}, 
"Right_Eye_Lower_Lid_Bend":{"cladName":"rightEye", "cladIndex":18}, 
"eyeCorner_R_outerUpper_X":{"cladName":"rightEye", "cladIndex":9}, 
"Eye_Corner_R_Outer_Upper_Y":{"cladName":"rightEye", "cladIndex":10}, 
"Eye_Corner_R_Inner_Upper_X":{"cladName":"rightEye", "cladIndex":7}, 
"Eye_Corner_R_Inner_Lower_X":{"cladName":"rightEye", "cladIndex":5},
"Eye_Corner_R_Inner_Lower_Y":{"cladName":"rightEye", "cladIndex":6}, 
"Eye_Corner_R_Outer_Lower_X":{"cladName":"rightEye", "cladIndex":11}, 
"Eye_Corner_R_Outer_Lower_Y":{"cladName":"rightEye", "cladIndex":12}, 
"Eye_Corner_L_Outer_Upper_X":{"cladName":"leftEye", "cladIndex":9}, 
"Eye_Corner_L_Outer_Upper_Y":{"cladName":"leftEye", "cladIndex":10}, 
"Eye_Corner_L_Inner_Upper_X":{"cladName":"leftEye", "cladIndex":7}, 
"Eye_Corner_L_Inner_Upper_Y":{"cladName":"leftEye", "cladIndex":8}, 
"Eye_Corner_L_Inner_Lower_X":{"cladName":"leftEye", "cladIndex":5}, 
"Eye_Corner_L_Inner_Lower_Y":{"cladName":"leftEye", "cladIndex":6}, 
"Eye_Corner_L_Outer_Lower_X":{"cladName":"leftEye", "cladIndex":11}, 
"Eye_Corner_L_Outer_Lower_Y":{"cladName":"leftEye", "cladIndex":12}
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
        audioStart = cmds.sound(audioNode,query=True, sourceStart=True)
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

def GetMovement():
    global ANIM_FPS
    #Get a list of all the user defined attributes
    dataNodeObject = "x:mech_all_ctrl"
    if( len( cmds.ls(dataNodeObject) ) == 0 ):
        return None
    attributes = cmds.listAttr(dataNodeObject, ud=True)
    if( len( attributes ) == 0 ):
        return None
    json_arr = []
    
    keyframe_attr_data = {}
    #Movement requires all attributes be set with the known names TODO: better error handling
    keyframe_attr_data["RadiusValues"] = cmds.keyframe( dataNodeObject, attribute = "Radius", query=True, valueChange=True)
    keyframe_attr_data["TurnValues"] = cmds.keyframe( dataNodeObject, attribute = "Turn", query=True, valueChange=True)
    keyframe_attr_data["FwdValues"] = cmds.keyframe( dataNodeObject, attribute = "Forward", query=True, valueChange=True)
    keyframe_attr_data["Times"] = cmds.keyframe( dataNodeObject, attribute = "Radius", query=True, timeChange=True)

    if( keyframe_attr_data["Times"] is None):
        return None
    #Loop through all keyframes
    keyframe_count = len(keyframe_attr_data["Times"])
    for i in range(keyframe_count-1):
        #skip the ending frames and go to the start of the next bookend, we process in pairs.
        if( ( i % 2) != 0 ):
            continue
        duration = (keyframe_attr_data["Times"][i+1] - keyframe_attr_data["Times"][i]) * 1000 / ANIM_FPS
        triggerTime_ms = round((keyframe_attr_data["Times"][i]) * 1000  / ANIM_FPS, 0);
        durationTime_ms = round(duration, 0); 
        curr = {
                    "triggerTime_ms": triggerTime_ms,
                    "durationTime_ms": durationTime_ms,
                    "Name": "BodyMotionKeyFrame"
                }
        keyframe_attr_data["FwdValues"][i+1] = round(keyframe_attr_data["FwdValues"][i+1])
        keyframe_attr_data["RadiusValues"][i+1] = round(keyframe_attr_data["RadiusValues"][i+1])
        keyframe_attr_data["TurnValues"][i+1] = round(keyframe_attr_data["TurnValues"][i+1])
        if( (keyframe_attr_data["FwdValues"][i+1] != 0) and  
            (keyframe_attr_data["RadiusValues"][i+1] == 0) and
            (keyframe_attr_data["TurnValues"][i+1] == 0)):
            curr["radius_mm"] = "STRAIGHT"
            curr["speed"] =round(keyframe_attr_data["FwdValues"][i+1])
        elif( (keyframe_attr_data["FwdValues"][i+1] == 0) and  
            (keyframe_attr_data["RadiusValues"][i+1] == 0) and
            (keyframe_attr_data["TurnValues"][i+1] != 0)):
            curr["radius_mm"] = "TURN_IN_PLACE"
            curr["speed"] =round(keyframe_attr_data["TurnValues"][i+1])
        else:
            curr["radius_mm"] = keyframe_attr_data["RadiusValues"][i+1]
            curr["speed"] =round(keyframe_attr_data["TurnValues"][i+1])
        json_arr.append(curr)
    return json_arr

# function that adds ( creates new or modifies existing keyframe)
def AddProceduralKeyframe(currattr,triggerTime_ms,durationTime_ms,val):
    # search and see if we have something at that time
    # if exists just modify currattr value else insert a blank one.
    global g_ProceduralFaceKeyFrames
    global g_ProcFaceDict
    frame = None
    for existing_face_frame in g_ProceduralFaceKeyFrames:
        if( existing_face_frame["triggerTime_ms"] == triggerTime_ms):
            frame = existing_face_frame

    if( frame is None ):
        #add a completely empty one with logical defaults
        frame = {        
            "faceAngle": 0.0,
            "faceCenterX": 0,
            "faceCenterY": 0,
            "faceScaleX": 1.0,
            "faceScaleY": 1.0,
            "leftEye":  ([0]*19),
            "rightEye":  ([0]*19),
            "triggerTime_ms": triggerTime_ms,
            "durationTime_ms": durationTime_ms,
            "Name": "ProceduralFaceKeyFrame"
        }
        g_ProceduralFaceKeyFrames.append(frame)

    # mod the attribute you need
    #"Eye_Corner_L_Outer_Lower_X":{"cladName":"leftEye", "cladIndex":11}, 
    write_info = g_ProcFaceDict[currattr]
    if( write_info["cladIndex"] >= 0):
        frame[write_info["cladName"]][write_info["cladIndex"]] = val
    else:
        frame[write_info["cladName"]] = val
    return

#JSON printing
def ExportAnkiAnim(item):
    global g_AnkiExportPath;
    print "AnkiAnimExportStarted..."
    if( g_AnkiExportPath == ""):
        SetAnkiExportPath("Default");
    global ANIM_FPS
    #originally this was just grabbing the first selected object cmds.ls(sl=True)[0].split(':')[1]
    # Hardcoded on Mooly's request, in case of multiple cozmos will need a change to the x namespace.
    DATA_NODE_NAME = "x:data_node"
    if( len( cmds.ls(DATA_NODE_NAME) ) == 0 ):
        print "ERROR: AnkiAnimExport: Select an object to export."
        return
    dataNodeObject = cmds.ls(DATA_NODE_NAME)[0]
    animFrames = cmds.playbackOptions(query = True, animationEndTime = True)
    animLength_s = animFrames / ANIM_FPS

    #Bake the animation so that we get the maya values mapped to the cozmo face values that the datanode driven keyframes transform
    cmds.bakeResults( dataNodeObject, time=(1,cmds.playbackOptions( query=True,max=True )),simulation=True, smart=True,disableImplicitControl=True,preserveOutsideKeys=True,sparseAnimCurveBake=False,removeBakedAttributeFromLayer=False,removeBakedAnimFromLayer=False,bakeOnOverrideLayer=True,minimizeRotation=True,controlPoints=False,shape=True )   

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
        #Loop through all keyframes
        for k in Vs:
            #Duration is (timeN+1-timeN)
            #Last Duration is anim-length - timeN
            duration = 0
            if i<len(Vs)-1:
                duration = (Ts[i+1] - Ts[i]) * 1000 / ANIM_FPS 
            else:
                duration = (animFrames - Ts[i]) * 1000  / ANIM_FPS    
            curr = None
            triggerTime_ms = int(round((Ts[i]) * 1000  / ANIM_FPS, 0));
            durationTime_ms = int(round(duration, 0));
            if currattr == "HeadAngle":
                curr = {
                    "angle_deg": k,
                    "angleVariability_deg": 0,
                    "triggerTime_ms": triggerTime_ms,
                    "durationTime_ms": durationTime_ms,
                    "Name": "HeadAngleKeyFrame",
                }
            elif currattr == "ArmLift":
                curr = {          
                    "height_mm": k,
                    "heightVariability_mm": 0,
                    "triggerTime_ms": triggerTime_ms,
                    "durationTime_ms": durationTime_ms,
                    "Name": "LiftHeightKeyFrame"
                }
            elif isProceduralFaceAttr:
                AddProceduralKeyframe(currattr,triggerTime_ms,durationTime_ms,k)

            if( curr is not None):
                json_arr.append(curr)
            i = i+1
    #Concat the procedural face frames which were added per attribute
    json_arr.extend(g_ProceduralFaceKeyFrames)
    #Grab the robot sounds from the main timeline, not a datanode attribute
    audio_json = GetAudioJSON()
    if( audio_json is not None):
        json_arr.append(audio_json)
    #Grab the robot movement relative to the curve
    movement_json_arr = GetMovement()
    if( movement_json_arr is not None):
        json_arr.extend(movement_json_arr)
    #Deletes the results layer the script created.
    cmds.delete("BakeResults")
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
        ExportAnkiAnim()

# Creator
def cmdCreator():
    return OpenMayaMPx.asMPxPtr( scriptedCommand() )
    
# Initialize the script plug-in
def initializePlugin(mobject):
    global g_PrevMenuName
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    print "Initting Anki plugin"
    try:
        mplugin.registerCommand( kPluginCmdName, cmdCreator )
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
        
        



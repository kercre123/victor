import argparse
import json
import math
import os
import requests
import time

kProcFace_DefaultScanlineOpacity = 1

FACE_DISPLAY_WIDTH = 184
FACE_DISPLAY_HEIGHT = 96

FLOATING_POINT_COMPARISON_TOLERANCE_FLT = 1e-5

ANIM_TIME_STEP_MS = 33 # ms

EyeParamCombineMethod_None = 0
EyeParamCombineMethod_Add = 1
EyeParamCombineMethod_Multiply = 2
EyeParamCombineMethod_AverageIfBothUnset = 3

class EyeParamInfo:
  def __init__(self, name, isAngle, canBeUnset, defaultValue, defaultValueIfCombiningWithUnset, combineMethod, clipLimits_min, clipLimits_max):
    self.name = name
    self.isAngle = isAngle
    self.canBeUnset = canBeUnset # parameter can be 'unset', i.e. -1 (cannot use this if isAngle=True!)
    self.defaultValue = defaultValue # initial value for the parameter
    self.defaultValueIfCombiningWithUnset = defaultValueIfCombiningWithUnset # value to use as default when combining and both unset, ignored if canBeUnset=False
    self.combineMethod = combineMethod
    self.clipLimits_min = clipLimits_min
    self.clipLimits_max = clipLimits_max

kEyeParamInfoLUT = [
    EyeParamInfo('EyeCenterX',        False, False,  0.0, 0.0, EyeParamCombineMethod_Add,                -FACE_DISPLAY_WIDTH/2, FACE_DISPLAY_WIDTH/2),
    EyeParamInfo('EyeCenterY',        False, False,  0.0, 0.0, EyeParamCombineMethod_Add,                -FACE_DISPLAY_HEIGHT/2,FACE_DISPLAY_HEIGHT/2),
    EyeParamInfo('EyeScaleX',         False, False,  1.0, 0.0, EyeParamCombineMethod_Multiply,           0.0, float('inf')),
    EyeParamInfo('EyeScaleY',         False, False,  1.0, 0.0, EyeParamCombineMethod_Multiply,           0.0, float('inf')),
    EyeParamInfo('EyeAngle',          True,  False,  0.0, 0.0, EyeParamCombineMethod_Add,                -90,  90),
    EyeParamInfo('LowerInnerRadiusX', False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('LowerInnerRadiusY', False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('UpperInnerRadiusX', False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('UpperInnerRadiusY', False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('UpperOuterRadiusX', False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('UpperOuterRadiusY', False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('LowerOuterRadiusX', False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('LowerOuterRadiusY', False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('UpperLidY',         False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('UpperLidAngle',     True,  False,  0.0, 0.0, EyeParamCombineMethod_Add,                -45,  45),
    EyeParamInfo('UpperLidBend',      False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('LowerLidY',         False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('LowerLidAngle',     True,  False,  0.0, 0.0, EyeParamCombineMethod_Add,                -45,  45),
    EyeParamInfo('LowerLidBend',      False, False,  0.0, 0.0, EyeParamCombineMethod_None,               0.0, 1.0),
    EyeParamInfo('Saturation',        False, True,  -1.0, 1.0, EyeParamCombineMethod_None,               -1.0, 1.0),
    EyeParamInfo('Lightness',         False, True,  -1.0, 1.0, EyeParamCombineMethod_None,               -1.0, 1.0),
    EyeParamInfo('GlowSize',          False, True,  -1.0, 0.5, EyeParamCombineMethod_None,               -1.0, 1.0),
    EyeParamInfo('HotSpotCenterX',    False, True,   0.0, 0.0, EyeParamCombineMethod_AverageIfBothUnset, -1.0, 1.0),
    EyeParamInfo('HotSpotCenterY',    False, True,   0.0, 0.0, EyeParamCombineMethod_AverageIfBothUnset, -1.0, 1.0)
]

def DEG_TO_RAD(deg):
  return deg * 0.017453292519943295474

def RAD_TO_DEG(rad):
  return rad * 57.295779513082322865

def IsNear(x, y, epsilon = None):
  if epsilon == None:
    epsilon = FLOATING_POINT_COMPARISON_TOLERANCE_FLT

  return (abs(x-y) < epsilon)

def IsNearZero(x):
  return IsNear(x, 0.0, FLOATING_POINT_COMPARISON_TOLERANCE_FLT)

def IsFltGEZero(x):
  return (x >= -FLOATING_POINT_COMPARISON_TOLERANCE_FLT)

def LinearBlendHelper(value1, value2, blendFraction):
  if value1 == value2:
    # Special case, no math needed
    return value1

  blendValue = ((1.0 - blendFraction)*value1 + blendFraction*value2)
  return blendValue


TWO_PI = math.pi * 2

def BlendAngleHelper(angle1, angle2, blendFraction):
  if angle1 == angle2:
    # Special case, no math needed
    return angle1

  angle1_rad = DEG_TO_RAD(angle1)
  angle2_rad = DEG_TO_RAD(angle2)

  fromAngle = (angle1_rad + TWO_PI) % TWO_PI
  toAngle = (angle2_rad + TWO_PI) % TWO_PI

  diff = math.fabs(fromAngle - toAngle)
  if diff >= math.pi:
      if fromAngle > toAngle:
          fromAngle -= TWO_PI;
      else:
          toAngle -= TWO_PI;

  result = LinearBlendHelper(fromAngle, toAngle, blendFraction);
  return RAD_TO_DEG(result)


class ProceduralFace:
  def __init__(self):
    self._eyeParams = [[0.0] * len(kEyeParamInfoLUT), [0.0] * len(kEyeParamInfoLUT)]

    for whichEye in [0, 1]:
      for iParam in range(0, len(kEyeParamInfoLUT)):
        self._eyeParams[whichEye][iParam] = kEyeParamInfoLUT[iParam].defaultValue

    self._faceAngle_deg = 0.0
    self._faceCenter = [0.0, 0.0]
    self._faceScale = [1.0, 1.0]
    self._scanlineOpacity = kProcFace_DefaultScanlineOpacity

  def __str__(self):
    result = "Left:"+str(self._eyeParams[0])+"\n"
    result += "Right:"+str(self._eyeParams[1])+"\n"
    result += "Angle:"+str(self._faceAngle_deg)+"\n"
    result += "Center:"+str(self._faceCenter)+"\n"
    result += "Scale:"+str(self._faceScale)+"\n"
    result += "ScanlineOpacity:"+str(self._scanlineOpacity)+"\n"
    return result

  def Clip(self, eye, param, newValue):
    if newValue < kEyeParamInfoLUT[param].clipLimits_min:
      newValue = kEyeParamInfoLUT[param].clipLimits_min
    elif newValue > kEyeParamInfoLUT[param].clipLimits_max:
      newValue = kEyeParamInfoLUT[param].clipLimits_max

    return newValue

  def GetParameter(self, whichEye, param):
    return self._eyeParams[whichEye][param]

  def SetParameter(self, whichEye, param, value):
    self._eyeParams[whichEye][param] = self.Clip(whichEye, param, value)

  def GetFaceAngle(self):
    return self._faceAngle_deg

  def SetFaceAngle(self, angle_deg):
    self._faceAngle_deg = angle_deg

  def GetFacePosition(self):
    return self._faceCenter

  def SetFacePosition(self, position):
    self._faceCenter = position

  def SetFaceScale(self, scale):
    if scale[0] < 0.0:
      scale[0] = 0.0
    if scale[1] < 0.0:
      scale[1] = 0.0
    self._faceScale = scale

  def GetFaceScale(self):
    return self._faceScale

  def SetScanlineOpacity(self, opacity):
    self._scanlineOpacity = opacity

  def GetScanlineOpacity(self):
    return self._scanlineOpacity

  def Interpolate(self, face1, face2, blendFraction, usePupilSaccades = None):
    # Special cases, no blending required:
    if IsNearZero(blendFraction):
      return face1
    elif IsNear(blendFraction, 1.0):
      return face2
    for whichEye in [0,1]:
      for iParam in range(0, len(kEyeParamInfoLUT)):
        if kEyeParamInfoLUT[iParam].isAngle:
          # Treat this param as an angle
          self.SetParameter(whichEye, iParam,
                            BlendAngleHelper(face1.GetParameter(whichEye, iParam),
                                             face2.GetParameter(whichEye, iParam),
                                             blendFraction))
        elif kEyeParamInfoLUT[iParam].canBeUnset:
          # Special linear blend taking into account whether values are 'set'
          self.SetParameter(whichEye, iParam,
                            LinearBlendHelper(face1.GetParameter(whichEye, iParam),
                                              face2.GetParameter(whichEye, iParam),
                                              blendFraction))
        else:
          self.SetParameter(whichEye, iParam,
                            LinearBlendHelper(face1.GetParameter(whichEye, iParam),
                                              face2.GetParameter(whichEye, iParam),
                                              blendFraction))
    self.SetFaceAngle(BlendAngleHelper(face1.GetFaceAngle(),
                                       face2.GetFaceAngle(),
                                       blendFraction))
    self.SetFacePosition([LinearBlendHelper(face1.GetFacePosition()[0],
                                            face2.GetFacePosition()[0],
                                            blendFraction),
                          LinearBlendHelper(face1.GetFacePosition()[1],
                                            face2.GetFacePosition()[1],
                                            blendFraction)])
    self.SetFaceScale([LinearBlendHelper(face1.GetFaceScale()[0],
                                         face2.GetFaceScale()[0],
                                         blendFraction),
                       LinearBlendHelper(face1.GetFaceScale()[1],
                                         face2.GetFaceScale()[1],
                                         blendFraction)])
    self.SetScanlineOpacity(LinearBlendHelper(face1.GetScanlineOpacity(),
                                              face2.GetScanlineOpacity(),
                                              blendFraction))
    return self

  def PostToWebserver(self, address):
    url = 'http://'+address+':8889/consolevarset'
    data = []

    for param in range(0, len(kEyeParamInfoLUT)):
      value = self.GetParameter(0, param)
      value = int(value*1000.0)/1000.0
      data.append('key=Left_'+kEyeParamInfoLUT[param].name+'&value='+str(value))

      value = self.GetParameter(1, param)
      value = int(value*1000.0)/1000.0
      data.append('key=Right_'+kEyeParamInfoLUT[param].name+'&value='+str(value))

    value = self.GetFaceAngle()
    value = int(value*1000.0)/1000.0
    data.append("key=ProcFace_Angle_deg&value="+str(value))

    value = self.GetFaceScale()[0]
    value = int(value*1000.0)/1000.0
    data.append("key=ProcFace_ScaleX&value="+str(value))

    value = self.GetFaceScale()[1]
    value = int(value*1000.0)/1000.0
    data.append("key=ProcFace_ScaleY&value="+str(value))

    value = self.GetFacePosition()[0]
    value = int(value*1000.0)/1000.0
    data.append("key=ProcFace_CenterX&value="+str(value))

    value = self.GetFacePosition()[1]
    value = int(value*1000.0)/1000.0
    data.append("key=ProcFace_CenterY&value="+str(value))

    value = self.GetScanlineOpacity()
    value = int(value*1000.0)/1000.0
    data.append("key=ProcFace_ScanlineOpacity&value="+str(value))

    data = '&'.join(data)
    r = requests.post(url, data=data)

  def PostToTerminal(self,watch_param=None):
    data = []

    for param in range(0, len(kEyeParamInfoLUT)):
      if watch_param and watch_param not in kEyeParamInfoLUT[param].name:
        continue

      value = self.GetParameter(0, param)
      value = int(value*1000.0)/1000.0
      data.append('Left.'+kEyeParamInfoLUT[param].name+'='+str(value))

      value = self.GetParameter(1, param)
      value = int(value*1000.0)/1000.0
      data.append('Right.'+kEyeParamInfoLUT[param].name+'='+str(value))

    value = self.GetFaceAngle()
    value = int(value*1000.0)/1000.0
    data.append("FaceAngle="+str(value))

    x = self.GetFaceScale()[0]
    x = int(x*1000.0)/1000.0
    y = self.GetFaceScale()[1]
    y = int(y*1000.0)/1000.0
    data.append("FaceScale=("+str(x)+","+str(y)+")")

    x = self.GetFacePosition()[0]
    x = int(x*1000.0)/1000.0
    y = self.GetFacePosition()[1]
    y = int(y*1000.0)/1000.0
    data.append("FacePosition=("+str(x)+","+str(y)+")")

    value = self.GetScanlineOpacity()
    value = int(value*1000.0)/1000.0
    data.append("ScanlineOpacity="+str(value))

    print(data)

class Keyframe:
  def __init__(self, json):
    self.procFace = ProceduralFace()
    self.triggerTime_ms = json['triggerTime_ms']

    for eye in [0, 1]:
      if eye == 0:
        eyeStr = 'leftEye'
      else:
        eyeStr = 'rightEye'

      numParams = len(json[eyeStr])
      for iParam in range(0, numParams):
        self.procFace.SetParameter(eye, iParam, json[eyeStr][iParam])

      for iParam in range(numParams+1, len(kEyeParamInfoLUT)):
        if kEyeParamInfoLUT[iParam].canBeUnset:
          self.procFace.SetParameter(eye, iParam, kEyeParamInfoLUT[iParam].defaultValueIfCombiningWithUnset)

    self.procFace.SetFaceAngle(json['faceAngle'])
    self.procFace.SetFacePosition([json['faceCenterX'],json['faceCenterY']])
    self.procFace.SetFaceScale([json['faceScaleX'],json['faceScaleY']])
    self.procFace.SetScanlineOpacity(json['scanlineOpacity'])

  def __str__(self):
    text = 'time:'+str(self.triggerTime_ms)+'\n'
    text += str(self.procFace)
    return text

  def GetTriggerTime(self):
    return self.triggerTime_ms

  def IsTimeToPlay(self, startTime_ms, currTime_ms = None):
    if currTime_ms == None:
      return self.GetTriggerTime() <= startTime_ms
    else:
      return self.GetTriggerTime() + startTime_ms <= currTime_ms

  def GetInterpolatedFace(self, nextFrame, currentTime_ms):
      # The interpolation fraction is how far along in time we are from this frame's
      # trigger time (which currentTime was initialized to) and the next frame's
      # trigger time.
      fraction = (currentTime_ms - self.GetTriggerTime()) / float(nextFrame.GetTriggerTime() - self.GetTriggerTime())
      if (fraction > 1.0):
        fraction = 1.0
      
      interpFace = ProceduralFace()
      interpFace = interpFace.Interpolate(self.procFace, nextFrame.procFace, fraction)
      
      return interpFace

  def GetFace(self):
      return self.procFace


class Track:
  def __init__(self):
    self.keyframes = []
    self.keyframe = 0

  def __str__(self):
    text = []
    for key in self.keyframes:
      text.append(str(key))

    return '\n'.join(text)

  def Append(self, keyframe):
    self.keyframes.append(Keyframe(keyframe))

  def HasFramesLeft(self):
    return (self.keyframe < len(self.keyframes))

  def GetCurrentKeyFrame(self):
    return self.keyframes[self.keyframe]

  def GetNextKeyFrame(self):
    if (self.keyframe+1 < len(self.keyframes)):
      return self.keyframes[self.keyframe+1]
    else:
      return None

  def MoveToNextKeyFrame(self):
    # For canned frames, we just move to the next one in the track
    self.keyframe += 1


def GetFaceHelper(track, startTime_ms,  currTime_ms):
  currStreamTime = currTime_ms - startTime_ms
  if track.HasFramesLeft():
    currentKeyFrame = track.GetCurrentKeyFrame()
    if currentKeyFrame.IsTimeToPlay(startTime_ms, currTime_ms):
      interpolatedFace = ProceduralFace()
      
      nextFrame = track.GetNextKeyFrame()
      if nextFrame != None:
        if nextFrame.IsTimeToPlay(startTime_ms, currTime_ms):
          # Something is wrong. Just move to next frame...
          track.MoveToNextKeyFrame()
          
        else:
          interpolatedFace = currentKeyFrame.GetInterpolatedFace(nextFrame, currTime_ms - startTime_ms)

          if (nextFrame.IsTimeToPlay(startTime_ms, currTime_ms + ANIM_TIME_STEP_MS)):
            track.MoveToNextKeyFrame()
          
          return interpolatedFace
      else:
        # There's no next frame to interpolate towards: just send this keyframe
        #  and move forward
        interpolatedFace = currentKeyFrame.GetFace()
        track.MoveToNextKeyFrame()

        return interpolatedFace
      
  return None


def playOnRobot(options):
  # upload animation file to resources
  url = 'http://'+options.ipaddress+':8889/resources/assets/animations/'+options.file
  r = requests.put(url, data=open(options.file, 'rb').read())

  # register animation with canned animations
  url = 'http://'+options.ipaddress+':8889/consolefunccall?func=AddAnimation&args='+options.file
  r = requests.post(url)

  # trigger streaming animation
  url = 'http://'+options.ipaddress+':8889/consolefunccall?func=PlayAnimation&args='+options.animation
  r = requests.post(url)


def playOnHost(options):
    animation = json.load(open(options.file))
    anim = animation[options.animation]
    anim = sorted(anim, key=lambda time: time['triggerTime_ms'])
    tracks = {}
    for keyframe in anim:
      if keyframe['Name'] == 'ProceduralFaceKeyFrame':
        if keyframe['Name'] not in tracks:
          tracks[keyframe['Name']] = Track()
        tracks[keyframe['Name']].Append(keyframe)

    if options.ipaddress is not None:
      url = 'http://'+options.ipaddress+':8889/consolevarset'
      data = 'key=ProcFace_OverrideEyeParams&value=true'
      r = requests.post(url, data=data)

      url = 'http://'+options.ipaddress+':8889/consolefunccall?func=ResetFace'
      r = requests.post(url)

      url = 'http://'+options.ipaddress+':8889/consolefunccall?func=VictorFaceRenderer'
      r = requests.post(url)


    if options.realtime:
      scaleTime = 100.0
      startTime_ms = currTime_ms = time.time()*scaleTime
    else:
      startTime_ms = currTime_ms = 0

    track = tracks['ProceduralFaceKeyFrame']
    while track.HasFramesLeft():
      print(currTime_ms)

      face = GetFaceHelper(track, startTime_ms, currTime_ms)
      if face != None:
        if options.ipaddress is None:
          face.PostToTerminal() #'GlowSize'
        else:
          face.PostToWebserver(options.ipaddress)

      if options.realtime:
        currTime_ms = time.time()*scaleTime
      else:
        currTime_ms += 33

    if options.ipaddress is not None:
      url = 'http://'+options.ipaddress+':8889/consolevarset'
      data = 'key=ProcFace_OverrideEyeParams&value=false'
      r = requests.post(url, data=data)

def main():

  parser = argparse.ArgumentParser(description='playanimation tool for playing animation over wifi')

  parser.add_argument('--target', '-t',
                      action='store',
                      default='host',
                      help='Play the animation on the robot or host')

  parser.add_argument('--ipaddress', '-i',
                      action='store',
                      help="IP address of webots/robot")

  parser.add_argument('--file', '-f',
                      action='store',
                      required=True,
                      help="Animation JSON file and animation name unless otherwise specified")

  parser.add_argument('--animation', '-a',
                      action='store',
                      help="Name of animation within JSON/.bin file")

  parser.add_argument('--realtime', '-r',
                      action='store_true',
                      default=False,
                      help="Drive the animation on the host either at a fixed frame rate (default) or real-time")

  options = parser.parse_args()
  if options.animation == None:
    options.animation = os.path.splitext(os.path.split(options.file)[1])[0]

  if options.target == 'robot':
    playOnRobot(options)
  else:
    playOnHost(options)

if __name__ == "__main__":
  main()

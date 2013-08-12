% MATLAB controller for Webots
% File:          cozmo_matlab.m
% Date:          
% Description:   
% Author:        
% Modifications: 

% uncomment the next two lines if you want to use
% MATLAB's desktop to interact with the controller:
%desktop;
%keyboard;

doProfile = true;

if doProfile
    profile on
end

blockWorldController = BlockWorldWebotController();

addpath ../../../products-cozmo/matlab
initCozmo; % Set up paths for BlockWorld to work

blockWorld = BlockWorld('CameraType', 'webot', ...
    'CameraDevice', blockWorldController.cam_head, ...
    'CameraCalibration', blockWorldController.GetCalibrationStruct('cam_head'), ... 
    'MatCameraDevice', blockWorldController.cam_down, ...
    'MatCameraCalibration', blockWorldController.GetCalibrationStruct('cam_down'), ...
    'HasMat', true, 'ZDirection', 'down', ...
    'GroundTruthPoseFcn', @(name)GetGroundTruthPose(blockWorldController,name), ...
    'ObservedPoseFcn', @(name,pose,trans)UpdatePose(blockWorldController,name,pose,trans));

% main loop:
% perform simulation steps of TIME_STEP milliseconds
% and leave the loop when Webots signals the termination
%
while wb_robot_step(blockWorldController.TIME_STEP) ~= -1

    done = blockWorldController.ControlCozmo();
    
    blockWorld.update();
    blockWorld.draw();
    
    if done 
        if doProfile
            profile off
            profsave(profile('info'),'~/temp/SimulatedBlockWorldProfile')
        end
        
        break;
    end

end

% cleanup code goes here: write data to files, etc.

    
    
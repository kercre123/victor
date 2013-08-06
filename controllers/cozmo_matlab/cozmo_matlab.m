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


profile on

blockWorldController = BlockWorldWebotController();

addpath ../../../products-cozmo/matlab
initCozmo; % Set up paths for BlockWorld to work

blockWorld = BlockWorld('CameraType', 'webot', ...
    'CameraDevice', blockWorldController.cam_head, ...
    'CameraCalibration', blockWorldController.GetCalibrationStruct('cam_head'), ... 
    'MatCameraDevice', blockWorldController.cam_down, ...
    'MatCameraCalibration', blockWorldController.GetCalibrationStruct('cam_down'), ...
    'hasMat', true);

% main loop:
% perform simulation steps of TIME_STEP milliseconds
% and leave the loop when Webots signals the termination
%
while wb_robot_step(blockWorldController.TIME_STEP) ~= -1

    done = blockWorldController.ControlCozmo();
    
    blockWorld.update();
    blockWorld.draw();
    
    if done
        profile off
        profsave(profile('info'),'~/temp/SimulatedBlockWorldProfile') 
        break;
    end

end

% cleanup code goes here: write data to files, etc.

    
    
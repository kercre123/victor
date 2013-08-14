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

useMatlabDisplay = true;
doProfile = false;

if doProfile
    if ~useMatlabDisplay
        warning(['Profiling information will probably not save ' ...
            'successfully if useMatlabDisplay = false.']);
    end
    profile on
end

% Set up paths for BlockWorld to work (do this before creating the
% BlockWorldWebotController object, which may rely on things in the Cozmo
% path)
addpath ../../../products-cozmo/matlab
initCozmo; 

blockWorldController = BlockWorldWebotController();

blockWorld = BlockWorld('CameraType', 'webot', ...
    'CameraDevice', blockWorldController.cam_head, ...
    'CameraCalibration', blockWorldController.GetCalibrationStruct('cam_head'), ... 
    'MatCameraDevice', blockWorldController.cam_down, ...
    'MatCameraCalibration', blockWorldController.GetCalibrationStruct('cam_down'), ...
    'HasMat', true, 'ZDirection', 'down', ...
    'GroundTruthPoseFcn', @(name)GetNodePose(blockWorldController,name), ...
    'UpdateObservedBlockPoseFcn', @(blockID,pose,trans)UpdateClosestBlockObservation(blockWorldController,blockID,pose,trans));

% main loop:
% perform simulation steps of TIME_STEP milliseconds
% and leave the loop when Webots signals the termination
%
while wb_robot_step(blockWorldController.TIME_STEP) ~= -1

    done = blockWorldController.ControlCozmo();
    
    blockWorld.update();
    
    if useMatlabDisplay
        blockWorld.draw('drawWorld', false, 'drawReprojection', false, ...
            'drawOverheadMap', true);
        
        if done
            if doProfile
                profile off
                profsave(profile('info'),'~/temp/SimulatedBlockWorldProfile')
            end
            
            break;
        end
    end

end

% cleanup code goes here: write data to files, etc.

    
    
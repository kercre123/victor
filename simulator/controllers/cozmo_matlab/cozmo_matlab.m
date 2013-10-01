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

% Set up paths for BlockWorld to work (do this before creating the
% BlockWorldWebotController object, which may rely on things in the Cozmo
% path)
run(fullfile('..', '..', '..', 'products-cozmo', 'matlab', 'initCozmoPath')); 

useMatlabDisplay = false;
doProfile = false;
headAngleSigma = .1; % Noise in head pitch angle measurement, in degrees
liftAngleSigma = .1; % Noise in lift angle measurement, in degrees
embeddedConversions = EmbeddedConversionsManager();
% embeddedConversions = EmbeddedConversionsManager( ...
%     'homographyEstimationType', 'opencv_cp2tform', ...
%     'computeCharacteristicScaleImageType', 'c_fixedPoint', ...
%     'traceBoundaryType', 'matlab_approximate', ...
%     'connectedComponentsType', 'matlab_approximate');

if doProfile
    if ~useMatlabDisplay
        warning(['Profiling information will probably not save ' ...
            'successfully if useMatlabDisplay = false.']);
    end
    profile on
end


blockWorldController = BlockWorldWebotController();

blockWorld = BlockWorld('CameraType', 'webot', ...
    'CameraDevice', blockWorldController.cam_head, ...
    'CameraCalibration', blockWorldController.GetCalibrationStruct('cam_head'), ...
    'EmbeddedConversions', embeddedConversions, ...
    'MatCameraDevice', blockWorldController.cam_down, ...
    'MatCameraCalibration', blockWorldController.GetCalibrationStruct('cam_down'), ...
    'HasMat', true, 'ZDirection', 'down', ...
    'GroundTruthPoseFcn', @(name)GetNodePose(blockWorldController,name), ...
    'UpdateObservedBlockPoseFcn', @(blockID,pose,color,trans)UpdateClosestBlockObservation(blockWorldController,blockID,pose,color,trans), ...
    'GetHeadPitchFcn', @()GetHeadAngle(blockWorldController, headAngleSigma), ...
    'SetHeadPitchFcn', @(angle)SetHeadAngle(blockWorldController, angle), ...
    'GetLiftAngleFcn', @()GetLiftAngle(blockWorldController, liftAngleSigma), ...
    'SetLiftAngleFcn', @(angle)SetLiftAngle(blockWorldController, angle), ...
    'SetLiftPositionFcn', @(pos)SetLiftPosition(blockWorldController, pos), ...
    'IsBlockLockedFcn', @()blockWorldController.locked, ...
    'ReleaseBlockFcn', @()ReleaseBlock(blockWorldController), ...
    'DisplayMessageFcn', @(type,message)DisplayMessage(blockWorldController, type, message), ...
    'DriveFcn', @(left,right)SetAngularWheelVelocity(blockWorldController, left, right));

blockWorldController.setOperationModeFcn = @(mode)setOperationMode(blockWorld, 1, mode);
blockWorldController.getOperationModeFcn = @()getOperationMode(blockWorld, 1);

% main loop:
% perform simulation steps of TIME_STEP milliseconds
% and leave the loop when Webots signals the termination
%
while wb_robot_step(blockWorldController.TIME_STEP) ~= -1

    done = blockWorldController.ControlCozmo();
    
    blockWorld.update();
    
    if useMatlabDisplay
        blockWorld.draw('drawWorld', true, 'drawReprojection', true, ...
            'drawOverheadMap', false);
                
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

    
    
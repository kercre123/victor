%% Init
rootPath = 'lib/anki/cozmo-engine/simulator/worlds/';
if ~isdir(rootPath)
    mkdir(rootPath)
end

WHEEL_RADIUS = 14.2;
BLOCK_DIM = 44;



%% TwoBlocksOnePose
% One robot looking at one mat marker and two sides of each of two blocks 
worldName = 'TwoBlocksOnePose';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']));

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', Pose([0 0 0], [105 67 0]), ...
    'HeadAngle', -18*pi/180, ...
    'CameraResolution', [320 240]);

TestWorld.AddBlock('BlockPose', Pose(-pi/3*[0 0 1], [270 32+67 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'angryFace', 'MarkerImg', 'symbols/angryFace');

TestWorld.AddBlock('BlockPose', Pose(pi/3*[0 0 1], [270 -32+67 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

TestWorld.Run();

%% PoseClustering
% One robot seeing two markers on the same object, which should trigger
% clustering the clustering of those two computed poses into one

worldName = 'PoseCluster';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']), ...
    'CheckRobotPose', false);

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', Pose([0 0 0], [0 0 0]), ...
    'HeadAngle', -10*pi/180, ...
    'CameraResolution', [320 240]);

TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [150 0 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'angryFace', 'MarkerImg', 'symbols/angryFace');

TestWorld.Run();


%% MatPoseTest
% No blocks, just testing robot pose estimation as it moves around to
% different poses on a mat
worldName = 'MatPoseTest';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']));

poses = { ...
    Pose( 0.26 *[0 0 1],  [ -66 -231 0]), ...
    Pose( .4927*[0 0 1],  [-161 -114 0]), ...
    Pose(     0*[0 0 1],  [  81 -66.4 0]), ...
    Pose(-2.466*[0 0 1],  [ 302  302 0]), ...
    Pose( 1.995*[0 0 1],  [-153 -289 0]), ...
    Pose( 1.914*[0 0 1],  [ -12 -30  0]), ...
    Pose( -0.564*[0 0 1], [-323 -9   0]) };
    
headAngles = [-22 -22 -12 -14 -10 -12 0] * pi/180;

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', poses, ...
    'HeadAngle', headAngles); 

TestWorld.Run();

%% RepeatedBlock
% Lots of blocks of the same type
worldName = 'RepeatedBlock';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']), ...
    'CheckRobotPose', false);

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', Pose([0 0 0], [110 0 0]), 'HeadAngle', 8*pi/180);

% Right stack
TestWorld.AddBlock('BlockPose', Pose(-pi/6*[0 0 1], [315 -75 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

TestWorld.AddBlock('BlockPose', Pose(-pi/6*[0 0 1], [315 -75 BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

TestWorld.AddBlock('BlockPose', Pose(-pi/6*[0 0 1], [315 -75 2*BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

% Left stack
TestWorld.AddBlock('BlockPose', Pose(pi/6*[0 0 1], [315 75 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

TestWorld.AddBlock('BlockPose', Pose(pi/6*[0 0 1], [315 75 BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

TestWorld.AddBlock('BlockPose', Pose(pi/6*[0 0 1], [315 75 2*BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

% Back stack
TestWorld.AddBlock('BlockPose', Pose(pi/2*[1 0 0], [300 0 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

TestWorld.AddBlock('BlockPose', Pose(pi*[1 0 0], [300 0 BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

TestWorld.AddBlock('BlockPose', Pose(3*pi/2*[1 0 0], [300 0 2*BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'bullseye2', 'MarkerImg', 'symbols/bullseye2');

TestWorld.Run();


%% VaryingDistance
% Looking at 1 block, as the robot moves away and looks up

worldName = 'VaryingDistance';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']), ...
    'CheckRobotPose', false);

poses = {};
%d = 0:5:350;
d = linspace(0, 200, 5);
for i = 1:length(d)
    poses{end+1} = Pose(0*[0 0 1], [-d(i) 0 0]); %#ok<SAGROW>
end
    
headAngles = linspace(-.3, -.05, length(poses));

% Repeat the distances with the robot slightly turned
theta = linspace(4, 22, length(poses)) * pi/180;
for i = 1:length(theta)
    poses{end+1} = Pose((theta(i))*[0 0 1], [-d(i) 0 0]); %#ok<SAGROW>
end

headAngles = [headAngles headAngles];

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', poses, ...
    'HeadAngle', headAngles, 'CameraResolution', [320 240]);

TestWorld.AddBlock('BlockPose', Pose(0*[0 0 1], [80 0 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'star5', 'MarkerImg', 'symbols/star5');

TestWorld.Run();

%% OffTheMat
% Resting on top of some blocks, looking down and seeing mat plus other
% blocks from above

worldName = 'OffTheMat';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']));

% 4 unobserved (type 0) blocks together, clustered around the specified
% center for Cozmo to sit on
center = [-30 -150 BLOCK_DIM/2];
TestWorld.AddBlock('BlockPose', Pose([0 0 0], center+[-BLOCK_DIM/2 -BLOCK_DIM/2 0]), ...
    'BlockProto', 'Block1x1');%, 'BlockName', 'sqtarget', 'FrontFace', 'symbols/squarePlusCorners');
TestWorld.AddBlock('BlockPose', Pose([0 0 0], center+[-BLOCK_DIM/2  BLOCK_DIM/2 0]), ...
    'BlockProto', 'Block1x1');%, 'BlockName', 'sqtarget', 'FrontFace', 'symbols/squarePlusCorners');
TestWorld.AddBlock('BlockPose', Pose([0 0 0], center+[ BLOCK_DIM/2 -BLOCK_DIM/2 0]), ...
    'BlockProto', 'Block1x1');%, 'BlockName', 'sqtarget', 'FrontFace', 'symbols/squarePlusCorners');
TestWorld.AddBlock('BlockPose', Pose([0 0 0], center+[ BLOCK_DIM/2  BLOCK_DIM/2 0]), ...
    'BlockProto', 'Block1x1');%, 'BlockName', 'sqtarget', 'FrontFace', 'symbols/squarePlusCorners');

% One Block for him to see
TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [145 60 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockName', 'fire', 'MarkerImg', 'symbols/fire');

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', Pose(pi/3*[0 0 1], center+[15 35 BLOCK_DIM/2]), ...
    'HeadAngle', -22*pi/180, 'CameraResolution', [320 240]);

TestWorld.Run();

%% SingleRamp
% Puts a ramp in the world and makes sure we can see it from all sides

worldName = 'SingleRamp';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']), ...
    'CheckRobotPose', false);

TestWorld.AddRamp();

poses = cell(1,4);
headAngles = -5*pi/180 * ones(1,4);
viewingDistance = 100;
poses{1} = Pose(pi*[0 0 1],      [ 100+viewingDistance   0  0]);
poses{2} = Pose( pi/2*[0 0 1],   [  0 -viewingDistance  0]);
poses{3} = Pose(-pi/2*[0 0 1],   [  0  viewingDistance  0]);
poses{4} = Pose(0*[0 0 1],       [-viewingDistance   0  0]);

TestWorld.AddRobot('Name', 'Cozmo_1', 'RobotPose', poses, ...
    'HeadAngle', headAngles, 'CameraResolution', [320 240]);

TestWorld.Run();



%% Init
rootPath = '../robot/simulator/worlds/';
if ~isdir(rootPath)
    mkdir(rootPath)
end

WHEEL_RADIUS = 14.2;
BLOCK_DIM = 44;

% NOTE: the BlockTypes used below must match those defined in BlockDefinitions.h!

%% TwoBlocksOnePose
% One robot looking at one mat marker and two sides of each of two blocks 
worldName = 'TwoBlocksOnePose';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']));

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', Pose([0 0 0], [95 67 0]), ...
    'HeadAngle', -18*pi/180, ...
    'CameraResolution', [320 240]);

TestWorld.AddBlock('BlockPose', Pose(-pi/3*[0 0 1], [270 32+67 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 1, 'FrontFace', 'batteries');

TestWorld.AddBlock('BlockPose', Pose(pi/3*[0 0 1], [270 -32+67 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.Run();

%% MatPoseTest
% No blocks, just testing robot pose estimation as it moves around to
% different poses on a mat
worldName = 'MatPoseTest';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']));

poses = { ...
    Pose( 0.26 *[0 0 1],  [ -96 -241 0]), ...
    Pose( .4927*[0 0 1],  [-161 -114 0]), ...
    Pose(     0*[0 0 1],  [  81 -6.4 0]), ...
    Pose(-2.466*[0 0 1],  [ 352  324 0]), ...
    Pose( 1.995*[0 0 1],  [-126 -378 0]), ...
    Pose( 1.914*[0 0 1],  [17.7 -109 0]), ...
    Pose( -0.564*[0 0 1], [-377  111 0]) };
    
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
    'RobotPose', Pose([0 0 0], [50 0 0]), 'HeadAngle', 5*pi/180);

% Right stack
TestWorld.AddBlock('BlockPose', Pose(-pi/4*[0 0 1], [333 -75 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(-pi/4*[0 0 1], [333 -75 BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(-pi/4*[0 0 1], [333 -75 2*BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

% Left stack
TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [333 75 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [333 75 BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [333 75 2*BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

% Back stack
TestWorld.AddBlock('BlockPose', Pose(pi/2*[1 0 0], [414 0 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(pi*[1 0 0], [414 0 BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(3*pi/2*[1 0 0], [414 0 2*BLOCK_DIM + BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.Run();


%% VaryingDistance
% Looking at 1 block, as the robot moves away and looks up

worldName = 'VaryingDistance';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']), ...
    'CheckRobotPose', false);

poses = {};
%d = 0:5:350;
d = 0:50:250;
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
    'HeadAngle', headAngles, 'CameraResolution', 2*[320 240]);

TestWorld.AddBlock('BlockPose', Pose(0*[0 0 1], [80 0 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 5, 'FrontFace', 'angryFace');

TestWorld.Run();

%% OffTheMat
% Resting on top of some blocks, looking down and seeing mat plus other
% blocks from above

worldName = 'OffTheMat';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']));

% 4 unobserved (type 0) blocks together, clustered around the specified
% center for Cozmo to sit on
center = [-30 -180 BLOCK_DIM/2];
TestWorld.AddBlock('BlockPose', Pose([0 0 0], center+[-BLOCK_DIM/2 -BLOCK_DIM/2 0]), ...
    'BlockProto', 'Block1x1', 'BlockType', 0);
TestWorld.AddBlock('BlockPose', Pose([0 0 0], center+[-BLOCK_DIM/2  BLOCK_DIM/2 0]), ...
    'BlockProto', 'Block1x1', 'BlockType', 0);
TestWorld.AddBlock('BlockPose', Pose([0 0 0], center+[ BLOCK_DIM/2 -BLOCK_DIM/2 0]), ...
    'BlockProto', 'Block1x1', 'BlockType', 0);
TestWorld.AddBlock('BlockPose', Pose([0 0 0], center+[ BLOCK_DIM/2  BLOCK_DIM/2 0]), ...
    'BlockProto', 'Block1x1', 'BlockType', 0);

% One Block for him to see
TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [145 60 BLOCK_DIM/2]), ...
    'BlockProto', 'Block1x1', 'BlockType', 3, 'FrontFace', 'fire');

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', Pose(pi/3*[0 0 1], center+[15 35 BLOCK_DIM/2]), ...
    'HeadAngle', -22*pi/180, 'CameraResolution', [320 240]);

TestWorld.Run();
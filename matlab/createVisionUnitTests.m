%% Init
rootPath = '../robot/simulator/worlds/';
if ~isdir(rootPath)
    mkdir(rootPath)
end

%% TwoBlocksOnePose
% One robot looking at one mat marker and two sides of each of two blocks 
worldName = 'TwoBlocksOnePose';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']));

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', Pose([0 0 0], [80 0 20]), ...
    'HeadAngle', -10*pi/180);

TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [333 75 30]), ...
    'BlockProto', 'Block1x1', 'BlockType', 1, 'FrontFace', 'batteryMarker');

TestWorld.AddBlock('BlockPose', Pose(-pi/4*[0 0 1], [333 -75 30]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.Run();

%% MatPoseTest
% No blocks, just testing robot pose estimation as it moves around to
% different poses on a mat
worldName = 'MatPoseTest';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']));

poses = { ...
    Pose( 0*[0 0 1],      [-170 -260 18]), ...
    Pose( 0.4927*[0 0 1], [-163  -95 18]), ...
    Pose(-.3927*[0 0 1], [-180  327 18]), ...
    Pose( 3.5344*[0 0 1], [421    63 18]), ...
    Pose(-1.5708*[0 0 1], [256 417 18]), ...
    Pose( .608*[0 0 1], [104 -370 18]), ...
    Pose( .608*[0 0 1], [-400 -338 18]), ...
    Pose( .608*[0 0 1], [-382 -89 18])};
    
headAngles = [-10 -15 -12 -10 -14 -12 -10 -23] * pi/180;

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
    'RobotPose', Pose([0 0 0], [0 0 18]), 'HeadAngle', 5*pi/180);

% Right stack
TestWorld.AddBlock('BlockPose', Pose(-pi/4*[0 0 1], [333 -75 30]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(-pi/4*[0 0 1], [333 -75 90]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(-pi/4*[0 0 1], [333 -75 150]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

% Left stack
TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [333 75 30]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [333 75 90]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(pi/4*[0 0 1], [333 75 150]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

% Back stack
TestWorld.AddBlock('BlockPose', Pose(pi/2*[1 0 0], [414 0 30]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(pi*[1 0 0], [414 0 90]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.AddBlock('BlockPose', Pose(3*pi/2*[1 0 0], [414 0 150]), ...
    'BlockProto', 'Block1x1', 'BlockType', 2, 'FrontFace', 'bullseye');

TestWorld.Run();


%% VaryingDistance
% Looking at 1 block, as the robot moves away and looks up

worldName = 'VaryingDistance';
TestWorld = CozmoTestWorldCreator('WorldFile', ...
    fullfile(rootPath, ['visionTest_' worldName '.wbt']), ...
    'CheckRobotPose', false);

poses = {};
d = 0:50:400;
for i = 1:length(d)
    poses{end+1} = Pose(pi*[0 0 1], [d(i) 0 18]); %#ok<SAGROW>
end
    
headAngles = linspace(-.3, 0, length(poses));

% Repeat the distances with the robot slightly turned
theta = linspace(4, 22, length(poses)) * pi/180;
for i = 1:length(theta)
    poses{end+1} = Pose((pi+theta(i))*[0 0 1], [d(i) 0 18]); %#ok<SAGROW>
end

headAngles = [headAngles headAngles];

TestWorld.AddRobot('Name', 'Cozmo_1', ...
    'RobotPose', poses, ...
    'HeadAngle', headAngles);

TestWorld.AddBlock('BlockPose', Pose(0*[0 0 1], [-100 0 30]), ...
    'BlockProto', 'Block1x1', 'BlockType', 1, 'FrontFace', 'batteryMarker');

TestWorld.Run();
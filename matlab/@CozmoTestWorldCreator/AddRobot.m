function AddRobot(this, varargin)

Name      = 'Cozmo_1';
RobotPose = {Pose()};
CommsChannel   = 1;

parseVarargin(varargin{:});

if ~iscell(RobotPose)
    RobotPose = {RobotPose};
end

underscoreIndex = strfind(Name, '_');
if isempty(underscoreIndex) || ...
        isnan(str2double(Name((underscoreIndex+1):end)))
    error('Name must end in _<ID>!');
end

fid = fopen(this.worldFile, 'at');
cleanup = onCleanup(@()fclose(fid));

% Add the cozmo bot
fprintf(fid, [...
    'DEF %s CozmoBot {\n' ...
    '  name "%s"\n' ...
    '  commsChannel %d\n' ...
    '  usbChannel   %d\n' ...
    '  controller "visionUnitTestController"\n', ...
    '  controllerArgs "', ...    
    ], Name, Name, CommsChannel, this.usbChannel);


% Pass the list of poses as controllerArgs
for i_pose = 1:length(RobotPose)
    T = RobotPose{i_pose}.T/1000;
    fprintf(fid, '%f %f %f %f %f %f %f ', ...
        RobotPose{i_pose}.axis(1), RobotPose{i_pose}.axis(2), RobotPose{i_pose}.axis(3), ...
        RobotPose{i_pose}.angle, ...
        T(1), T(2), T(3));
end

fprintf(fid, '"\n}\n\n'); 

if this.useOffboardVision
    % Add an offboard vision processor for this robot
    fprintf(fid, [ ...
        'DEF OffboardVisionProcessor USBConnection {\n' ...
        '  controller "offboard_vision_processor"\n' ...
        '  usbChannel %d\n' ...
        '  robotName "%s"\n' ...
        '}\n\n'], this.usbChannel, Name);
end % IF useOffboardVision

end % function AddRobot() 

    

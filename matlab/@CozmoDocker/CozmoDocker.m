classdef CozmoDocker < handle

    properties(Constant = true)
        NECK_JOINT_POSITION = [-15  0   45]; % relative to robot origin
        HEAD_CAM_ROTATION   = [0 0 1; -1 0 0; 0 -1 0]; % (rodrigues(-pi/2*[0 1 0])*rodrigues(pi/2*[1 0 0]))'
        HEAD_CAM_POSITION   = [ 20  0  -10]; % relative to neck joint
        LIFT_DISTANCE       = 34;  % forward from robot origin
        
        PLACE_BLOCKTYPE = 50; % Orange block with black sticky corners
        DOCK_BLOCKTYPE  = 60; % Yellow block with no sticky corners
    end
    
    properties
        h_fig;
        h_axes;
        h_pip;
        h_angleError;
        h_distError;
        h_leftRightError;
        h_img;
        h_target;
        h_txMsg, h_rxMsg;
        
        escapePressed;
        camera;
        calibration;
        
        detectionResolution;
        trackingResolution;
        
        trackerType;
        mode; 
        
        dockingDistance; % in mm, from camera
        
        % For homography-based pose estimation
        marker3d;
        H_init;
        K; % calibration matrix
        
        frameCount;
        numEmpty;
        numLost;
        
        drawPose;
        h_cam;
        h_robot;
        h_camPoseAxes;
        h_robotPoseAxes;
        
        % Poses
        robotPose;
        neckPose;
        headCamPose;
        blockPose;
    end
    
    
    methods
        function this = CozmoDocker(varargin)
            
            Port = 'COM4';
            BaudRate = 1500000;
            DetectionResolution = 'QVGA';
            TrackingResolution  = 'QQQVGA';
            Calibration = []; % at Tracking resolution!
            TrackerType = 'affine';
            DockDistanceMM = 30; % distance to top bar. this depends on head angle!
            HeadAngle = 0; % in degrees!
            DrawPose = true;
            
            parseVarargin(varargin{:});
            
            this.calibration         = Calibration;
            this.detectionResolution = DetectionResolution;
            this.trackingResolution  = TrackingResolution;
            this.trackerType         = TrackerType;
            
            this.camera              = SerialCamera(Port, 'BaudRate', BaudRate);
            this.escapePressed       = false;
            
            this.dockingDistance     = DockDistanceMM;
            this.drawPose            = DrawPose;
            
            this.mode = 'DOCK';
            
            % Set up Robot head camera geometry
            this.robotPose = Pose();
            this.neckPose = Pose([0 0 0], CozmoDocker.NECK_JOINT_POSITION);
            this.neckPose.parent = this.robotPose;
            
            this.setHeadAngle(HeadAngle * pi/180);
            
            % Set up display figure/axes:
            this.h_fig = namedFigure('Docking', 'CurrentCharacter', '~', ...
                'KeyPressFcn', @(src,edata)this.keyPressCallback(src,edata));
            
            clf(this.h_fig);
            
            this.h_rxMsg = annotation('textbox', [.2   0 .6 .05]);
            this.h_txMsg = annotation('textbox', [.2 .05 .6 .05]);
            set(this.h_rxMsg, 'String', '<RX message>', 'FontSize', 9);
            set(this.h_txMsg, 'String', '<TX message>', 'FontSize', 9);
            
            this.h_axes = axes('Pos', [.15 .15 .7 .8], 'Parent', this.h_fig); % subplot(1,1,1, 'Parent', this.h_fig);
            this.h_pip = axes('Position', [0 .75 .2 .2], 'Parent', this.h_fig);
            title(this.h_pip, 'Target');
            
            % Angle Error Plot
            ERRORBAR_LINEWIDTH = 25;
            h = axes('Position', [0 0 .2 .2], 'Parent', this.h_fig);
            title(h, 'Angle Error')
            this.h_angleError = plot([0 0], [0 0], 'r', 'Parent', h, 'LineWidth', ERRORBAR_LINEWIDTH);
            set(h, 'YTick', [], 'XTick', -30:5:30, ...
                'XTickLabel', [], 'YTickLabel', [], ...
                'XLim', 30*[-1 1], 'YLim', .25*[-1 1], 'XGrid', 'on');
            
            % Distance Error Plot
            h = axes('Position', [.8 0 .2 .2], 'Parent', this.h_fig);
            title(h, 'Distance');
            this.h_distError = plot([0 0], [0 0], 'r', 'Parent', h, 'LineWidth', ERRORBAR_LINEWIDTH);
            set(h, 'XTick', [], 'YTick', 0:20:150, ...
                'XTickLabel', [], 'YTickLabel', [], ...
                'XLim', .25*[-1 1], 'YLim', [0 150], 'YGrid', 'on');
            
            % Horizontal Error Plot
            h = axes('Position', [.8 .75 .2 .2], 'Parent', this.h_fig);
            title(h, 'Horizontal Error');
            %this.h_leftRightError = barh(0, 'Parent', h);
            this.h_leftRightError = plot([0 0], [0 0], 'r', 'Parent', h, 'LineWidth', ERRORBAR_LINEWIDTH);
            set(h, 'XTick', -20:5:20, 'YTick', [], ...
                'XTickLabel', [], 'YTickLabel', [], ...
                'XLim', [-20 20], 'YLim', .25*[-1 1], 'XGrid', 'on');
           
            this.h_img = imagesc(zeros(240,320), 'Parent', this.h_axes);
            axis(this.h_axes, 'image');
            colormap(this.h_fig, gray);
            hold(this.h_axes, 'on');
            this.h_target = plot(nan, nan, 'r', 'LineWidth', 2, 'Parent', this.h_axes);
            hold(this.h_axes, 'off');
            
            if strcmp(this.trackerType, 'homography')
                if ~isempty(this.calibration)
                    % For convenience, create a 3x3 camera calibration matrix
                    this.K = [this.calibration.fc(1) 0 this.calibration.cc(1); ...
                        0 this.calibration.fc(2) this.calibration.cc(2); ...
                        0 0 1];
                end
                
                WIDTH = BlockMarker3D.ReferenceWidth;
                this.marker3d = WIDTH/2 * [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
                    
                if this.drawPose
                    h_poseFig = namedFigure('Docking Pose', 'KeyPressFcn', ...
                        @(src,edata)this.keyPressCallback(src,edata));
                    this.h_camPoseAxes = subplot(1,2,1, 'Parent', h_poseFig);
                    
                    hold(this.h_camPoseAxes, 'off')
                    order = [1 2 4 3 1];
                    plot3(this.marker3d(order,1), this.marker3d(order,2), ...
                        this.marker3d(order,3), 'b', 'Parent', this.h_camPoseAxes);
                    hold(this.h_camPoseAxes, 'on')
                    block3d = 30*[-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
                    plot3(block3d(order,1), block3d(order,2), block3d(order,3), ...
                        'g', 'Parent', this.h_camPoseAxes);
                    
                    axis(this.h_camPoseAxes, 'equal');
                    grid(this.h_camPoseAxes, 'on');
                    
                    set(this.h_camPoseAxes, 'XLim', [-50 50], ...
                        'YLim', [-50 50], 'ZLim', [-150 10], ...
                        'View', [0 0]);
                    
                    xlabel(this.h_camPoseAxes, 'X Error')
                    ylabel(this.h_camPoseAxes, 'Y Error')
                    zlabel(this.h_camPoseAxes, 'Z Error')
                    title(this.h_camPoseAxes, 'Camera Pose');
                    
                    this.h_robotPoseAxes = subplot(1,2,2, 'Parent', h_poseFig);
                    
                    hold(this.h_robotPoseAxes, 'off')
                    order = [1 2 3 4 1];
                    robot = [30 40 20; 30 -40 20; -90 -40 20; -90 40 20];
                    plot3(robot(order,1), robot(order,2), robot(order,3), ...
                        'b', 'Parent', this.h_robotPoseAxes);
                    %plot3(this.marker3d(order,3), -this.marker3d(order,1), ...
                    %    -this.marker3d(order,2), 'b', 'Parent', this.h_robotPoseAxes);
                    hold(this.h_robotPoseAxes, 'on')
                    %plot3(block3d(order,3), -block3d(order,1), -block3d(order,2), ...
                    %    'g', 'Parent', this.h_robotPoseAxes);
                    
                    axis(this.h_robotPoseAxes, 'equal');
                    grid(this.h_robotPoseAxes, 'on');
                    
                    set(this.h_robotPoseAxes, 'XLim', [-10 150], ...
                        'YLim', [-50 50], 'ZLim', [-50 50], ...
                        'View', [-90 90]);
                    
                    xlabel(this.h_robotPoseAxes, 'X Error')
                    ylabel(this.h_robotPoseAxes, 'Y Error')
                    zlabel(this.h_robotPoseAxes, 'Z Error')
                    title(this.h_robotPoseAxes, 'Robot Pose');
                end % IF drawPose
            end % IF homography
            
        end % CONSTRUCTOR CozmoDocker()
        
        function run(this)
            
            this.frameCount = uint16(0);
            this.escapePressed = false;
            dockingDone = false;
            
            while ~this.escapePressed && ~dockingDone
                         
                this.initDetection();
                
                % Wait until we get a valid marker
                % TODO: wait until we get the marker for the block we want
                % to pick up!
                marker = [];
                while ~this.escapePressed && isempty(marker)
                    t_detect = tic;
                    img = this.camera.getFrame();
                  
                    if ~isempty(this.camera.message)
                        this.displayMessage(this.camera.message);
                    end
                    
                    if ~isempty(img)
                        set(this.h_img, 'CData', img); drawnow;
                        marker = simpleDetector(img);
                        
                        for i_marker = 1:length(marker)
                            if marker{i_marker}.isValid 
                                switch(this.mode)
                                    case 'DOCK'
                                        if marker{i_marker}.blockType ~= CozmoDocker.DOCK_BLOCKTYPE
                                            marker{i_marker} = [];
                                        end
                                    case 'PLACE'
                                        if marker{i_marker}.blockType ~= CozmoDocker.PLACE_BLOCKTYPE
                                            marker{i_marker} = [];
                                        end
                                    otherwise
                                        error('Unrecognized mode "%s".', this.mode);
                                end
                            else
                                marker{i_marker} = [];
                            end
                        
                        end
                        
                        marker(cellfun(@isempty, marker)) = [];
                        
                    end
                    
                    title(this.h_axes, sprintf('Detecting: %.1f FPS', 1/toc(t_detect)));
                    drawnow
                end
                
                if ~this.escapePressed
                    LKtracker = this.initTracker(marker);
                    
                    this.numLost  = 0;
                    this.numEmpty = 0;
                    while this.numLost < 5 && ~this.escapePressed
                        this.track(LKtracker);
                        
                        if this.numLost==0 && ~isempty(this.calibration)
                            [distError, midPointErr, angleError] = this.computeError(LKtracker);
                            
                            % Send out the error packet over the serial
                            % camera line:
                            dockErrorPacket = [uint8('E') ...
                                typecast(swapbytes(single([distError midPointErr angleError])), 'uint8')];
                            assert(length(dockErrorPacket)==13, ...
                                'Expecting docking error packet to be 13 bytes long.');
                            
                            this.camera.sendMessage(dockErrorPacket);
                        end
                    end % WHILE not lost
                    
                end % IF escape not pressed
                
            end % while escape not pressed and not done docking
            
        end % FUNCTION run()
       
        function initDetection(this)
            % Set the camera's resolution for detection.
            this.camera.changeResolution(this.detectionResolution);
            dims = this.camera.framesize;
            set(this.h_axes, 'XLim', [.5 dims(1)+.5], 'YLim', [.5 dims(2)+.5]);
            set(this.h_img, 'CData', zeros(dims(2),dims(1)));
            set(this.h_target, 'XData', nan, 'YData', nan);
            drawnow
            
        end
        
        function marker = findMarker(this)
            
            img = this.camera.getFrame();
            
            if ~isempty(this.camera.message)
                this.displayMessage(this.camera.message);
            end
            
            if ~isempty(img)
                set(this.h_img, 'CData', img); drawnow;
                marker = simpleDetector(img);
            end
            title(this.h_axes, sprintf('Detecting: %.1f FPS', 1/toc(t_detect)));
            drawnow
        end
        
        function LKtracker = initTracking(this, marker)
            this.camera.changeResolution(this.trackingResolution);
            
            % Initialize the tracker with the full resolution first
            % image, but tell it we're going to do tracking at
            % QQQVGA.
            LKtracker = LucasKanadeTracker(img, marker{1}.unorderedCorners([3 1 4 2],:), ...
                'Type', this.trackerType, 'RidgeWeight', 1e-3, ...
                'DebugDisplay', false, 'UseBlurring', false, ...
                'UseNormalization', true, ...
                'TrackingResolution', this.camera.framesize);
            
            if strcmp(this.trackerType, 'homography') && ...
                    ~isempty(this.calibration)
                
                % Compute the planar homography:
                %{
                        Kfull = this.K * 4;
                        this.H_init = compute_homography( ...
                            Kfull\[marker{1}.corners';ones(1,4)], ...
                            this.marker3d(:,1:2)');
                        this.H_init = .25 * this.H_init;
                %}
                
                this.H_init = compute_homography( ...
                    this.K\[LKtracker.corners';ones(1,4)], ...
                    this.marker3d(:,1:2)');
                
                this.updateBlockPose(this.H_init);
                
                %[Rmat, T] = this.getCameraPose(LKtracker.corners, this.marker3d);
                if this.drawPose
                    this.drawCamera();
                end
            end
            
            % Show the target we'll be tracking
            imagesc(LKtracker.target{LKtracker.finestScale}, 'Parent', this.h_pip);
            axis(this.h_pip, 'image', 'off');
            
            set(this.h_axes, ...
                'XLim', [.5 this.camera.framesize(1)+.5], ...
                'YLim', [.5 this.camera.framesize(2)+.5]);
            %set(this.h_img, 'CData', imresize(img, [60 80], 'nearest'));
            %set(this.h_target, 'XData', corners([1 2 4 3 1],1), ...
            %    'YData', corners([1 2 4 3 1],2));
            %drawnow;
            
        end % FUNCTION initTracking()
        
        function [lost, numEmpty] = track(this, lost, numEmpty)
            t_track = tic;
            
            img = this.camera.getFrame();
            if ~isempty(this.camera.message)
                this.displayMessage(this.camera.message);
            end
            
            if isempty(img)
                this.numEmpty = this.numEmpty + 1;
                
                if numEmpty > 5
                    % Too many empty frames in a row
                    warning('Too many empty frames received. Resetting camera.');
                    this.camera.reset();
                end
            else
                this.frameCount = this.frameCount + 1;
                
                this.numEmpty = 0;
                set(this.h_img, 'CData', img);
                converged = LKtracker.track(img, ...
                    'MaxIterations', 50, ...
                    'ConvergenceTolerance', .25,...
                    'ErrorTolerance', 0.5);
                
                if converged
                    this.numLost = 0;
                    corners = LKtracker.corners;
                    
                    set(this.h_target, ...
                        'XData', corners([1 2 4 3 1],1), ...
                        'YData', corners([1 2 4 3 1],2));
                    title(this.h_pip, sprintf('TargetError = %.2f', LKtracker.err), 'Back', 'w');
                    
                else
                    this.numLost = this.numLost + 1;
                end
                
                title(this.h_axes, sprintf('Tracking: %.1f FPS', 1/toc(t_track)));
                drawnow
            end % IF / ELSE image is empty
            
        end % FUNCTION track()
       
        function [distError, midPointErr, angleError] = computeError(this, LKtracker)
             
            % Compute the error signal according to the current tracking result
            switch(this.trackerType)
                case 'affine'
                    corners = LKtracker.corners;
                    
                    % TODO: why do i still need this rotation fix code? I'm
                    % using the unordered marker corners above and i
                    % thought that would fix it...
                    %
                    % Block may be rotated with top side of marker not
                    % facing up, so reorient to make sure we got top
                    % corners
                    [th,~] = cart2pol(corners(:,1)-mean(corners(:,1)), corners(:,2)-mean(corners(:,2)));
                    [~,sortIndex] = sort(th);
                    
                    upperLeft = corners(sortIndex(1),:);
                    upperRight = corners(sortIndex(2),:);
                    
                    %upperLeft = corners(1,:);
                    %upperRight = corners(2,:);
                    
                    assert(upperRight(1) > upperLeft(1), ...%if upperRight(1) < upperLeft(1)
                        ['UpperRight corner should be to the right ' ...
                        'of the UpperLeft corner.']);
                    %keyboard
                    %end
                    
                    % Get the angle from vertical of the top bar of the
                    % marker we're tracking
                    L = sqrt(sum( (upperRight-upperLeft).^2) );
                    angleError = -asin( (upperRight(2)-upperLeft(2)) / L);
                    
                    currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;
                    distError = currentDistance - CozmoDocker.LIFT_DISTANCE;
                    
                    % TODO: should i be comparing to ncols/2 or calibration center?
                    midPointErr = -( (upperRight(1)+upperLeft(1))/2 - ...
                        this.trackingResolution(1)/2 );
                    midPointErr = midPointErr * currentDistance / this.calibration.fc(1);
                    
                case 'homography'
                    
                    % Note that LKtracker internally does recentering -- we
                    % need to factor that in here as well. Thus the C term.
                    C = [1 0 -LKtracker.xcen; 0 1 -LKtracker.ycen; 0 0 1];
                    H = this.K\(C\LKtracker.tform*C*this.K*this.H_init);
                    
                    % This computes the equivalent H, but is more expensive
                    % since it involves an SVD internally to compute the
                    % homography instead of just chaining together the
                    % initial homography with the current one already found
                    % by the tracker, as above.
                    %H = compute_homography(this.K\[LKtracker.corners';ones(1,4)], ...
                    %        this.marker3d(:,1:2)');
                    
                    this.updateBlockPose(H);
                    
                    if this.drawPose
                        this.drawCamera();
                        this.drawRobot();
                    end
                    
                    blockWRTrobot = this.blockPose.getWithRespectTo(this.robotPose);
                    distError     = blockWRTrobot.T(1) - CozmoDocker.LIFT_DISTANCE;
                    midPointErr   = blockWRTrobot.T(2);
                    angleError    = atan2(blockWRTrobot.Rmat(2,1), blockWRTrobot.Rmat(1,1)) + pi/2;
                    
                otherwise
                    error(['Not sure how to compute an error signal ' ...
                        'when using a %s tracker.'], this.trackerType);
            end % SWITCH(trackerType)
             
            % Update the error displays
            set(this.h_angleError, 'XData', [0 angleError*180/pi]);
            h = get(this.h_angleError, 'Parent');
            title(h, sprintf('AngleErr = %.1fdeg', angleError*180/pi), 'Back', 'w');
            
            set(this.h_distError, 'YData', [0 distError]);
            h = get(this.h_distError, 'Parent');
            title(h, sprintf('DistToGo = %.1fmm', distError), 'Back', 'w');
            
            set(this.h_leftRightError, 'XData', [0 midPointErr]);
            h = get(this.h_leftRightError, 'Parent');
            title(h, sprintf('LeftRightErr = %.1fmm', midPointErr), 'Back', 'w');
            
            % Display the message we're sending:
            %{ 
            Print hex values for debugging
            txMsg = sprintf('Dist=%.2f(0x%X%X%X%X), L/R=%.2f(0x%X%X%X%X), Rad=%.2f(0x%X%X%X%X)', ...
                distError, typecast(swapbytes(single(distError)), 'uint8'), ...
                midPointErr, typecast(swapbytes(single(midPointErr)), 'uint8'), ...
                angleError, typecast(swapbytes(single(angleError)), 'uint8'));
            %}
            if mod(this.frameCount, this.camera.fps)==0
                txMsg = sprintf('TX: Dist(x)=%.2fmm, L/R(y)=%.2fmm, Angle=%.3frad', ...
                    distError, midPointErr, angleError);
                
                set(this.h_txMsg, 'String', txMsg);
                fprintf('Message Sent: %s\n', txMsg);
            end
            
            
        end % FUNCTION computeError()
        
        function keyPressCallback(this, ~,edata)
            if isempty(edata.Modifier)
                switch(edata.Key)
                    case 'space'
                        this.run();
                        
                    case 'escape'
                        this.escapePressed = true;
                        
                    case 'd'
                        this.mode = 'DOCK';
                        
                    case 'p'
                        this.mode = 'PLACE';
                end
            end
        end % keyPressCallback()
        
        function drawCamera(this)
            tempPose = this.headCamPose.getWithRespectTo(this.blockPose);
                        
            scale = 5;
            verts = scale*[-1 -1 0; -1 1 0; 1 1 0; 1 -1 0; 0 0 -.5];
            verts = tempPose.applyTo(verts);
            order = [1 2; 2 3; 3 4; 4 1; 1 5; 2 5; 3 5; 4 5]';
            if isempty(this.h_cam)
                this.h_cam = plot3(verts(order,1), verts(order,2), verts(order,3), 'Parent', this.h_camPoseAxes);
            else
                set(this.h_cam, 'XData', verts(order,1), 'YData', verts(order,2), 'ZData', verts(order,3));
            end
         end % FUNCTION drawCamera()
        
        function drawRobot(this)
            %tempPose = this.blockPose.getWithRespectTo(this.robotPose);
          
            % Bounding box of robot in camera coordinates, centered around
            % the camera origin -- this gets rotated and recentered by the
            % application of tempPose.inv
            %verts = [40 0 30; -40 0 30; -40 0 -90; 40 0 -90];

            % Block vertices in world coordinates
            %verts = [0 30 30; 0 -30 30; 0 -30 -30; 0 30 -30];
            %verts = tempPose.applyTo(verts);
            
            block = this.marker3d /(BlockMarker3D.ReferenceWidth/2)*30;
            verts = this.blockPose.applyTo(block);
            
            order = [1 2 4 3 1];
            if isempty(this.h_robot)
                this.h_robot = plot3(verts(order,1), verts(order,2), verts(order,3), 'r', 'Parent', this.h_robotPoseAxes);
            else
                set(this.h_robot, 'XData', verts(order,1), 'YData', verts(order,2), 'ZData', verts(order,3));
            end
            
        end
        
        
        function updateBlockPose(this, varargin)
            
            % Compute pose of block w.r.t. camera
            if nargin == 2
                H = varargin{1};
                
                % De-embed the initial 3D pose from the homography:
                scale = mean([norm(H(:,1));norm(H(:,2))]);
                %scale = H(3,3);
                H = H/scale;
                
                u1 = H(:,1);
                u1 = u1 / norm(u1);
                u2 = H(:,2) - dot(u1,H(:,2)) * u1;
                u2 = u2 / norm(u2);
                u3 = cross(u1,u2);
                R = [u1 u2 u3];
                %Rvec = rodrigues(Rmat);
                T    = H(:,3);
                
            else
                warning('Should not need to call compute_extrinsic_init anymore!');
                
                pts2d = varargin{1};
                pts3d = varargin{2};
                
                [~,T,R] = compute_extrinsic_init(pts2d', pts3d', ...
                    this.calibration.fc, this.calibration.cc, zeros(4,1), 0);
            end
            
            % Now switch to block with respect to robot, instead of camera
            this.blockPose = Pose(R,T);
            this.blockPose.parent = this.headCamPose;
            this.blockPose = this.blockPose.getWithRespectTo(this.robotPose);
            
        end % updateBlockPose()
        
        
        function setHeadAngle(this, newHeadAngle)
            
            % Get rotation matrix for the current head pitch, rotating
            % around the Y axis (which points to robot's LEFT!)
            Rpitch = rodrigues(-newHeadAngle*[0 1 0]);
                        
            this.headCamPose = Pose( ...
                Rpitch * CozmoDocker.HEAD_CAM_ROTATION,  ...
                Rpitch * CozmoDocker.HEAD_CAM_POSITION(:));
            this.headCamPose.parent = this.neckPose;
            
        end
        
        function displayMessage(this, msg)
            set(this.h_rxMsg, 'String', ['RX: ' msg]);
            fprintf('Message Received: %s\n', msg);
            
            if ~isempty(strfind(msg, 'DONE DOCKING'))
                this.mode = 'PLACE';
            elseif ~isempty(strfind(msg, 'DONE PLACEMENT'))
                this.mode = 'DOCK';
            end
        end
            
    end % methods

end % classdef CozmoDocker

